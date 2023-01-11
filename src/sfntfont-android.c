/* sfnt format font driver for Android.

Copyright (C) 2023 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <config.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#include <android/api-level.h>

#include "androidterm.h"
#include "sfntfont.h"
#include "pdumper.h"
#include "blockinput.h"
#include "android.h"

/* Array of directories to search for system fonts.  */
const char *system_font_directories[] =
  {
    "/system/fonts",
  };

/* The font cache.  */
static Lisp_Object font_cache;



static unsigned int
sfntfont_android_saturate32 (unsigned int a, unsigned int b)
{
  unsigned int c;

  c = a + b;

  if (c < a)
    c = -1;

  return c;
}

/* Scale each of the four packed bytes in P in the low 16 bits of P by
   SCALE.  Return the result.

   SCALE is an integer between 0 and 256.  */

static unsigned int
sfntfont_android_scale32 (unsigned int scale, unsigned int p)
{
  uint32_t ag, rb;
  uint32_t scaled_ag, scaled_rb;

  ag = (p & 0xFF00FF00) >> 8;
  rb = (p & 0x00FF00FF);

  scaled_ag = (scale * ag) & 0xFF00FF00;
  scaled_rb = (scale * rb) >> 8 & 0x00FF00FF;

  return scaled_ag | scaled_rb;
}

static unsigned int
sfntfont_android_mul8x2 (unsigned int a8, unsigned int b32)
{
  unsigned int i;

  b32 &= 0xff00ff;
  i = a8 * b32 + 0x800080;

  return (i + ((i >> 8) & 0xff00ff)) >> 8 & 0xff00ff;
}

/* Blend two pixels SRC and DST without utilizing any control flow.
   SRC must be in premultiplied ARGB8888 format, and DST must be in
   premultiplied ABGR8888 format.  Value is in premultiplied ABGR8888
   format.  */

static unsigned int
sfntfont_android_blend (unsigned int src, unsigned int dst)
{
  unsigned int a, br_part, ag_part, src_rb, both;

  a = (src >> 24);
  br_part = sfntfont_android_mul8x2 (255 - a, dst);
  ag_part = sfntfont_android_mul8x2 (255 - a, dst >> 8) << 8;

  both = ag_part | br_part;

  /* Swizzle src.  */
  src_rb = src & 0x00ff00ff;
  src = src & ~0x00ff00ff;
  src |= (src_rb >> 16 | src_rb << 16);

  /* Saturating is unnecessary but helps find bugs.  */
  return sfntfont_android_saturate32 (both, src);
}

#define U255TO256(x) ((unsigned short) (x) + ((x) >> 7))

/* Blend two pixels SRC and DST without utilizing any control flow.
   Both SRC and DST are expected to be in premultiplied ARGB8888
   format.  Value is returned in premultiplied ARGB8888 format.  */

static unsigned int
sfntfont_android_blendrgb (unsigned int src, unsigned int dst)
{
  unsigned int a, rb_part, ag_part, both;

  a = (src >> 24);
  rb_part = sfntfont_android_mul8x2 (255 - a, dst);
  ag_part = sfntfont_android_mul8x2 (255 - a, dst >> 8) << 8;

  both = ag_part | rb_part;

  /* Saturating is unnecessary but helps find bugs.  */
  return sfntfont_android_saturate32 (both, src);
}

/* Composite the bitmap described by BUFFER, STRIDE and TEXT_RECTANGLE
   onto the native-endian ABGR8888 bitmap described by DEST and
   BITMAP_INFO.  RECT is the subset of the bitmap to composite.  */

static void
sfntfont_android_composite_bitmap (unsigned char *restrict buffer,
				   size_t stride,
				   unsigned char *restrict dest,
				   AndroidBitmapInfo *bitmap_info,
				   struct android_rectangle *text_rectangle,
				   struct android_rectangle *rect)
{
  unsigned int *src_row;
  unsigned int *dst_row;
  unsigned int i, src_y, x, src_x, max_x, dst_x;

  if ((intptr_t) dest & 3 || bitmap_info->stride & 3)
    /* This shouldn't be possible as Android is supposed to align the
       bitmap to at least a 4 byte boundary.  */
    emacs_abort ();
  else
    {
      for (i = 0; i < rect->height; ++i)
	{
	  if (i + rect->y >= bitmap_info->height)
	    /* Done.  */
	    return;

	  src_y = i + (rect->y - text_rectangle->y);

	  src_row = (unsigned int *) ((buffer + src_y * stride));
	  dst_row = (unsigned int *) (dest + ((i + rect->y)
					      * bitmap_info->stride));

	  /* Figure out where the loop below should end.  */
	  max_x = min (rect->width, bitmap_info->width - rect->x);

	  /* Keep this loop simple! */
	  for (x = 0; x < max_x; ++x)
	    {
	      src_x = x + (rect->x - text_rectangle->x);
	      dst_x = x + rect->x;

	      dst_row[dst_x]
		= sfntfont_android_blend (src_row[src_x],
					  dst_row[dst_x]);
	    }
	}
    }
}

/* Calculate the union containing both A and B, both boxes.  Place the
   result in RESULT.  */

static void
sfntfont_android_union_boxes (struct gui_box a, struct gui_box b,
			      struct gui_box *result)
{
  result->x1 = min (a.x1, b.x1);
  result->y1 = min (a.y1, b.y1);
  result->x2 = max (a.x2, b.x2);
  result->y2 = max (a.y2, b.y2);
}

/* Draw the specified glyph rasters from FROM to TO on behalf of S,
   using S->gc.  Fill the background if WITH_BACKGROUND is true.

   See init_sfntfont_vendor and sfntfont_draw for more details.  */

static void
sfntfont_android_put_glyphs (struct glyph_string *s, int from,
			     int to, int x, int y, bool with_background,
			     struct sfnt_raster **rasters,
			     int *x_coords)
{
  struct android_rectangle background, text_rectangle, rect;
  struct gui_box text, character;
  unsigned int *buffer, *row;
  unsigned char *restrict raster_row;
  size_t stride, i;
  AndroidBitmapInfo bitmap_info;
  unsigned char *bitmap_data;
  jobject bitmap;
  int left, top, temp_y;
  unsigned int prod, raster_y;

  if (!s->gc->num_clip_rects)
    /* Clip region is empty.  */
    return;

  if (from == to)
    /* Nothing to draw.  */
    return;

  USE_SAFE_ALLOCA;

  prepare_face_for_display (s->f, s->face);

  /* Build the scanline buffer.  Figure out the bounds of the
     background.  */
  memset (&background, 0, sizeof background);

  if (with_background)
    {
      background.x = x;
      background.y = y - FONT_BASE (s->font);
      background.width = s->width;
      background.height = FONT_HEIGHT (s->font);
    }

  /* Now figure out the bounds of the text.  */

  if (rasters[0])
    {
      text.x1 = x_coords[0] + rasters[0]->offx;
      text.x2 = text.x1 + rasters[0]->width;
      text.y1 = y - (rasters[0]->height + rasters[0]->offy);
      text.y2 = y - rasters[0]->offy;
    }
  else
    memset (&text, 0, sizeof text);

  for (i = 1; i < to - from; ++i)
    {
      /* See if text has to be extended.  */

      if (!rasters[i])
	continue;

      character.x1 = x_coords[i] + rasters[i]->offx;
      character.x2 = character.x1 + rasters[i]->width;
      character.y1 = y - (rasters[i]->height + rasters[i]->offy);
      character.y2 = y - rasters[i]->offy;

      sfntfont_android_union_boxes (text, character, &text);
    }

  /* Union the background rect with the text rectangle.  */
  text_rectangle.x = text.x1;
  text_rectangle.y = text.y1;
  text_rectangle.width = text.x2 - text.x1;
  text_rectangle.height = text.y2 - text.y1;
  gui_union_rectangles (&background, &text_rectangle,
			&text_rectangle);

  /* Allocate enough to hold text_rectangle.height, aligned to 8
     bytes.  Then fill it with the background.  */
  stride = (text_rectangle.width * sizeof *buffer) + 7 & ~7;
  SAFE_NALLOCA (buffer, text_rectangle.height, stride);
  memset (buffer, 0, text_rectangle.height * stride);

  if (with_background)
    {
      /* Fill the background.  First, offset the background rectangle
	 to become relative from text_rectangle.x,
	 text_rectangle.y.  */
      background.x = background.x - text_rectangle.x;
      background.y = background.y - text_rectangle.y;
      eassert (background.x >= 0 && background.y >= 0);

      for (temp_y = background.y; (temp_y
				   < (background.y
				      + background.height));
	   ++temp_y)
	{
	  row = (unsigned int *) ((unsigned char *) buffer
				  + stride * temp_y);

	  for (x = background.x; x < background.x + background.width; ++x)
	    row[x] = s->gc->background | 0xff000000;
	}
    }

  /* Draw all the rasters onto the buffer.  */
  for (i = 0; i < to - from; ++i)
    {
      if (!rasters[i])
	continue;

      /* Figure out the top and left of the raster relative to
	 text_rectangle.  */
      left = x_coords[i] + rasters[i]->offx - text_rectangle.x;

      /* Note that negative offy represents the part of the text that
	 lies below the baseline.  */
      top = (y - (rasters[i]->height + rasters[i]->offy)
	     - text_rectangle.y);
      eassert (left >= 0 && top >= 0);

      /* Draw the raster onto the temporary bitmap using the
	 foreground color scaled by the alpha map.  */

      for (raster_y = 0; raster_y < rasters[i]->height; ++raster_y)
	{
	  row = (unsigned int *) ((unsigned char *) buffer
				  + stride * (raster_y + top));
	  raster_row = &rasters[i]->cells[raster_y * rasters[i]->stride];

	  for (x = 0; x < rasters[i]->width; ++x)
	    {
	      prod
		= sfntfont_android_scale32 (U255TO256 (raster_row[x]),
					    (s->gc->foreground
					     | 0xff000000));
	      row[left + x]
		= sfntfont_android_blendrgb (prod, row[left + x]);
	    }
	}
    }

  /* Lock the bitmap.  It must be unlocked later.  */
  bitmap_data = android_lock_bitmap (FRAME_ANDROID_WINDOW (s->f),
				     &bitmap_info, &bitmap);

  /* If locking the bitmap fails, just discard the data that was
     allocated.  */
  if (!bitmap_data)
    {
      SAFE_FREE ();
      return;
    }

  /* Loop over each clip rect in the GC.  */
  eassert (bitmap_info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);

  if (s->gc->num_clip_rects > 0)
    {
      for (i = 0; i < s->gc->num_clip_rects; ++i)
	{
	  if (!gui_intersect_rectangles (&s->gc->clip_rects[i],
					 &text_rectangle, &rect))
	    /* Outside the clip region.  */
	    continue;

	  /* Composite the intersection onto the buffer.  */
	  sfntfont_android_composite_bitmap ((unsigned char *) buffer,
					     stride, bitmap_data,
					     &bitmap_info,
					     &text_rectangle, &rect);
	}
    }
  else /* gc->num_clip_rects < 0 */
    sfntfont_android_composite_bitmap ((unsigned char *) buffer,
				       stride, bitmap_data,
				       &bitmap_info,
				       &text_rectangle,
				       &text_rectangle);

  /* Release the bitmap.  */
  AndroidBitmap_unlockPixels (android_java_env, bitmap);
  ANDROID_DELETE_LOCAL_REF (bitmap);

  /* Damage the window by the text rectangle.  */
  android_damage_window (FRAME_ANDROID_WINDOW (s->f),
			 &text_rectangle);

  /* Release the temporary scanline buffer.  */
  SAFE_FREE ();
}



/* Font driver definition.  */

/* Return the font cache for this font driver.  F is ignored.  */

static Lisp_Object
sfntfont_android_get_cache (struct frame *f)
{
  return font_cache;
}

/* The Android sfntfont driver.  */
const struct font_driver android_sfntfont_driver =
  {
    .type = LISPSYM_INITIALLY (Qsfnt_android),
    .case_sensitive = true,
    .get_cache = sfntfont_android_get_cache,
    .list = sfntfont_list,
    .match = sfntfont_match,
    .draw = sfntfont_draw,
    .open_font = sfntfont_open,
    .close_font = sfntfont_close,
    .encode_char = sfntfont_encode_char,
    .text_extents = sfntfont_text_extents,
    .list_family = sfntfont_list_family,

    /* TODO: list_family, shaping.  */
  };



/* This is an ugly hack that should go away, but I can't think of
   how.  */

DEFUN ("android-enumerate-fonts", Fandroid_enumerate_fonts,
       Sandroid_enumerate_fonts, 0, 0, 0,
       doc: /* Enumerate fonts present on the system.

Signal an error if fonts have already been enumerated.  This would
normally have been done in C, but reading fonts require Lisp to be
loaded before character sets are made available.  */)
  (void)
{
  DIR *dir;
  int i;
  struct dirent *dirent;
  char name[PATH_MAX * 2];
  static bool enumerated;

  if (enumerated)
    error ("Fonts have already been enumerated");
  enumerated = true;

  block_input ();

  /* Scan through each of the system font directories.  Enumerate each
     font that looks like a TrueType font.  */
  for (i = 0; i < ARRAYELTS (system_font_directories); ++i)
    {
      dir = opendir (system_font_directories[i]);

      if (!dir)
	continue;

      while ((dirent = readdir (dir)))
	{
	  /* If it contains (not ends with!) with .ttf, then enumerate
	     it.  */

	  if (strstr (dirent->d_name, ".ttf"))
	    {
	      sprintf (name, "%s/%s", system_font_directories[i],
		       dirent->d_name);
	      sfnt_enum_font (name);
	    }
	}

      closedir (dir);
    }

  unblock_input ();

  return Qnil;
}



static void
syms_of_sfntfont_android_for_pdumper (void)
{
  init_sfntfont_vendor (Qsfnt_android, &android_sfntfont_driver,
			sfntfont_android_put_glyphs);
  register_font_driver (&android_sfntfont_driver, NULL);
}

void
init_sfntfont_android (void)
{
  /* Make sure to pick the right Sans Serif font depending on what
     version of Android the device is running.  */
  if (android_get_device_api_level () >= 15)
    Vsfnt_default_family_alist
      = list2 (Fcons (build_string ("Monospace"),
		      build_string ("Droid Sans Mono")),
	       Fcons (build_string ("Sans Serif"),
		      build_string ("Roboto")));
  else
    Vsfnt_default_family_alist
      = list2 (Fcons (build_string ("Monospace"),
		      build_string ("Droid Sans Mono")),
	       Fcons (build_string ("Sans Serif"),
		      build_string ("Droid Sans")));
}

void
syms_of_sfntfont_android (void)
{
  DEFSYM (Qsfnt_android, "sfnt-android");
  DEFSYM (Qandroid_enumerate_fonts, "android-enumerate-fonts");
  Fput (Qandroid, Qfont_driver_superseded_by, Qsfnt_android);

  font_cache = list (Qnil);
  staticpro (&font_cache);

  defsubr (&Sandroid_enumerate_fonts);

  pdumper_do_now_and_after_load (syms_of_sfntfont_android_for_pdumper);
}
