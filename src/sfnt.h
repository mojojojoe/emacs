/* sfnt format font support for GNU Emacs.

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

#ifndef _SFNT_H_
#define _SFNT_H_

#include <stdint.h>
#include <stddef.h>



/* Container structure and enumerator definitions.  */

/* The sfnt container format is organized into different tables, such
   as ``cmap'' or ``glyf''.  Each of these tables has a specific
   format and use.  These are all the tables known to Emacs.  */

enum sfnt_table
  {
    SFNT_TABLE_CMAP,
    SFNT_TABLE_GLYF,
    SFNT_TABLE_HEAD,
    SFNT_TABLE_HHEA,
    SFNT_TABLE_HMTX,
    SFNT_TABLE_LOCA,
    SFNT_TABLE_MAXP,
    SFNT_TABLE_NAME,
    SFNT_TABLE_META,
  };

#define SFNT_ENDOF(type, field, type1)			\
  ((size_t) offsetof (type, field) + sizeof (type1))

/* Each of these structures must be aligned so that no compiler will
   ever generate padding bytes on platforms where the alignment
   requirements for uint32_t and uint16_t are no larger than 4 and 2
   bytes respectively.  */

struct sfnt_offset_subtable
{
  /* The scaler type.  */
  uint32_t scaler_type;

  /* The number of tables.  */
  uint16_t num_tables;

  /* (Maximum power of 2 <= numTables) * 16.  */
  uint16_t search_range;

  /* log2 (maximum power of 2 <= numTables) */
  uint16_t entry_selector;

  /* numTables * 16 - searchRange.  */
  uint16_t range_shift;

  /* Variable length data.  */
  struct sfnt_table_directory *subtables;
};

/* The table directory.  Follows the offset subtable, with one for
   each table.  */

struct sfnt_table_directory
{
  /* 4-byte identifier for each table.  See sfnt_table_names.  */
  uint32_t tag;

  /* Table checksum.  */
  uint32_t checksum;

  /* Offset from the start of the file.  */
  uint32_t offset;

  /* Length of the table in bytes, not subject to padding.  */
  uint32_t length;
};

enum sfnt_scaler_type
  {
    SFNT_SCALER_TRUE = 0x74727565,
    SFNT_SCALER_VER1 = 0x00010000,
    SFNT_SCALER_TYP1 = 0x74797031,
    SFNT_SCALER_OTTO = 0x4F54544F,
  };

typedef int32_t sfnt_fixed;
typedef int16_t sfnt_fword;
typedef uint16_t sfnt_ufword;

#define sfnt_coerce_fixed(fixed) ((sfnt_fixed) (fixed) / 65535.0)

typedef unsigned int sfnt_glyph;
typedef unsigned int sfnt_char;

struct sfnt_head_table
{
  /* The version.  This is a 16.16 fixed point number.  */
  sfnt_fixed version;

  /* The revision.  */
  sfnt_fixed revision;

  /* Checksum adjustment.  */
  uint32_t checksum_adjustment;

  /* Magic number, should be 0x5F0F3CF5.  */
  uint32_t magic;

  /* Flags for the font.  */
  uint16_t flags;

  /* Units per em.  */
  uint16_t units_per_em;

  /* Time of creation.  */
  uint32_t created_high, created_low;

  /* Time of modification.  */
  uint32_t modified_high, modified_low;

  /* Minimum bounds.  */
  sfnt_fword xmin, ymin, xmax, ymax;

  /* Mac specific stuff.  */
  uint16_t mac_style;

  /* Smallest readable size in pixels.  */
  uint16_t lowest_rec_ppem;

  /* Font direction hint.  */
  int16_t font_direction_hint;

  /* Index to loc format.  0 for short offsets, 1 for long.  */
  int16_t index_to_loc_format;

  /* Unused.  */
  int16_t glyph_data_format;
};

struct sfnt_hhea_table
{
  /* The version.  This is a 16.16 fixed point number.  */
  sfnt_fixed version;

  /* The maximum ascent and descent values for this font.  */
  sfnt_fword ascent, descent;

  /* The typographic line gap.  */
  sfnt_fword line_gap;

  /* The maximum advance width.  */
  sfnt_ufword advance_width_max;

  /* The minimum bearings on either side.  */
  sfnt_fword min_left_side_bearing, min_right_side_bearing;

  /* The maximum extent.  */
  sfnt_fword x_max_extent;

  /* Caret slope.  */
  int16_t caret_slope_rise, caret_slope_run;

  /* Caret offset for non slanted fonts.  */
  sfnt_fword caret_offset;

  /* Reserved values.  */
  int16_t reserved1, reserved2, reserved3, reserved4;

  /* Should always be zero.  */
  int16_t metric_data_format;

  /* Number of advanced widths in metrics table.  */
  uint16_t num_of_long_hor_metrics;
};

struct sfnt_cmap_table
{
  /* Should be zero.  */
  uint16_t version;

  /* Number of subtables.  */
  uint16_t num_subtables;
};

enum sfnt_platform_id
  {
    SFNT_PLATFORM_UNICODE   = 0,
    SFNT_PLATFORM_MACINTOSH = 1,
    SFNT_PLATFORM_RESERVED  = 2,
    SFNT_PLATFORM_MICROSOFT = 3,
  };

enum sfnt_unicode_platform_specific_id
  {
    SFNT_UNICODE_1_0		     = 0,
    SFNT_UNICODE_1_1		     = 1,
    SFNT_UNICODE_ISO_10646_1993	     = 2,
    SFNT_UNICODE_2_0_BMP	     = 3,
    SFNT_UNICODE_2_0		     = 4,
    SFNT_UNICODE_VARIATION_SEQUENCES = 5,
    SFNT_UNICODE_LAST_RESORT	     = 6,
  };

enum sfnt_macintosh_platform_specific_id
  {
    SFNT_MACINTOSH_ROMAN	       = 0,
    SFNT_MACINTOSH_JAPANESE	       = 1,
    SFNT_MACINTOSH_TRADITIONAL_CHINESE = 2,
    SFNT_MACINTOSH_KOREAN	       = 3,
    SFNT_MACINTOSH_ARABIC	       = 4,
    SFNT_MACINTOSH_HEBREW	       = 5,
    SFNT_MACINTOSH_GREEK	       = 6,
    SFNT_MACINTOSH_RUSSIAN	       = 7,
    SFNT_MACINTOSH_RSYMBOL	       = 8,
    SFNT_MACINTOSH_DEVANGARI	       = 9,
    SFNT_MACINTOSH_GURMUKHI	       = 10,
    SFNT_MACINTOSH_GUJARATI	       = 11,
    SFNT_MACINTOSH_ORIYA	       = 12,
    SFNT_MACINTOSH_BENGALI	       = 13,
    SFNT_MACINTOSH_TAMIL	       = 14,
    SFNT_MACINTOSH_TELUGU	       = 15,
    SFNT_MACINTOSH_KANNADA	       = 16,
    SFNT_MACINTOSH_MALAYALAM	       = 17,
    SFNT_MACINTOSH_SINHALESE	       = 18,
    SFNT_MACINTOSH_BURMESE	       = 19,
    SFNT_MACINTOSH_KHMER	       = 20,
    SFNT_MACINTOSH_THAI		       = 21,
    SFNT_MACINTOSH_LAOTIAN	       = 22,
    SFNT_MACINTOSH_GEORGIAN	       = 23,
    SFNT_MACINTOSH_ARMENIAN	       = 24,
    SFNT_MACINTOSH_SIMPLIFIED_CHINESE  = 25,
    SFNT_MACINTOSH_TIBETIAN	       = 26,
    SFNT_MACINTOSH_MONGOLIAN	       = 27,
    SFNT_MACINTOSH_GEEZ		       = 28,
    SFNT_MACINTOSH_SLAVIC	       = 29,
    SFNT_MACINTOSH_VIETNAMESE	       = 30,
    SFNT_MACINTOSH_SINDHI	       = 31,
    SFNT_MACINTOSH_UNINTERPRETED       = 32,
  };

enum sfnt_microsoft_platform_specific_id
  {
    SFNT_MICROSOFT_SYMBOL	 = 0,
    SFNT_MICROSOFT_UNICODE_BMP	 = 1,
    SFNT_MICROSOFT_SHIFT_JIS	 = 2,
    SFNT_MICROSOFT_PRC		 = 3,
    SFNT_MICROSOFT_BIG_FIVE	 = 4,
    SFNT_MICROSOFT_WANSUNG	 = 5,
    SFNT_MICROSOFT_JOHAB	 = 6,
    SFNT_MICROSOFT_UNICODE_UCS_4 = 10,
  };

struct sfnt_cmap_encoding_subtable
{
  /* The platform ID.  */
  uint16_t platform_id;

  /* Platform specific ID.  */
  uint16_t platform_specific_id;

  /* Mapping table offset.  */
  uint32_t offset;
};

struct sfnt_cmap_encoding_subtable_data
{
  /* Format and possibly the length in bytes.  */
  uint16_t format, length;
};

struct sfnt_cmap_format_0
{
  /* Format, set to 0.  */
  uint16_t format;

  /* Length in bytes.  Should be 262.  */
  uint16_t length;

  /* Language code.  */
  uint16_t language;

  /* Character code to glyph index map.  */
  uint8_t glyph_index_array[256];
};

struct sfnt_cmap_format_2_subheader
{
  uint16_t first_code;
  uint16_t entry_count;
  int16_t id_delta;
  uint16_t id_range_offset;
};

struct sfnt_cmap_format_2
{
  /* Format, set to 2.  */
  uint16_t format;

  /* Length in bytes.  */
  uint16_t length;

  /* Language code.  */
  uint16_t language;

  /* Array mapping high bytes to subheaders.  */
  uint16_t sub_header_keys[256];

  /* Variable length data.  */
  struct sfnt_cmap_format_2_subheader *subheaders;
  uint16_t *glyph_index_array;
  uint16_t num_glyphs;
};

struct sfnt_cmap_format_4
{
  /* Format, set to 4.  */
  uint16_t format;

  /* Length in bytes.  */
  uint16_t length;

  /* Language code.  */
  uint16_t language;

  /* 2 * seg_count.  */
  uint16_t seg_count_x2;

  /* 2 * (2**FLOOR(log2(segCount))) */
  uint16_t search_range;

  /* log2(searchRange/2) */
  uint16_t entry_selector;

  /* Variable-length data.  */
  uint16_t *end_code;
  uint16_t *reserved_pad;
  uint16_t *start_code;
  int16_t *id_delta;
  int16_t *id_range_offset;
  uint16_t *glyph_index_array;

  /* The number of elements in glyph_index_array.  */
  size_t glyph_index_size;
};

struct sfnt_cmap_format_6
{
  /* Format, set to 6.  */
  uint16_t format;

  /* Length in bytes.  */
  uint16_t length;

  /* Language code.  */
  uint16_t language;

  /* First character code in subrange.  */
  uint16_t first_code;

  /* Number of character codes.  */
  uint16_t entry_count;

  /* Variable-length data.  */
  uint16_t *glyph_index_array;
};

struct sfnt_cmap_format_8_or_12_group
{
  uint32_t start_char_code;
  uint32_t end_char_code;
  uint32_t start_glyph_code;
};

struct sfnt_cmap_format_8
{
  /* Format, set to 8.  */
  uint16_t format;

  /* Reserved.  */
  uint16_t reserved;

  /* Length in bytes.  */
  uint32_t length;

  /* Language code.  */
  uint32_t language;

  /* Tightly packed array of bits (8K bytes total) indicating whether
     the particular 16-bit (index) value is the start of a 32-bit
     character code.  */
  uint8_t is32[65536];

  /* Number of groups.  */
  uint32_t num_groups;

  /* Variable length data.  */
  struct sfnt_cmap_format_8_or_12_group *groups;
};

/* cmap formats 10, 13 and 14 unsupported.  */

struct sfnt_cmap_format_12
{
  /* Format, set to 12.  */
  uint16_t format;

  /* Reserved.  */
  uint16_t reserved;

  /* Length in bytes.  */
  uint32_t length;

  /* Language code.  */
  uint32_t language;

  /* Number of groups.  */
  uint32_t num_groups;

  /* Variable length data.  */
  struct sfnt_cmap_format_8_or_12_group *groups;
};

struct sfnt_maxp_table
{
  /* Table version.  */
  sfnt_fixed version;

  /* The number of glyphs in this font - 1.  Set at version 0.5 or
     later.  */
  uint16_t num_glyphs;

  /* These fields are only set in version 1.0 or later.  Maximum
     points in a non-composite glyph.  */
  uint16_t max_points;

  /* Maximum contours in a non-composite glyph.  */
  uint16_t max_contours;

  /* Maximum points in a composite glyph.  */
  uint16_t max_composite_points;

  /* Maximum contours in a composite glyph.  */
  uint16_t max_composite_contours;

  /* 1 if instructions do not use the twilight zone (Z0), or 2 if
     instructions do use Z0; should be set to 2 in most cases.  */
  uint16_t max_zones;

  /* Maximum points used in Z0.  */
  uint16_t max_twilight_points;

  /* Number of Storage Area locations.  */
  uint16_t max_storage;

  /* Number of FDEFs, equal to the highest function number + 1.  */
  uint16_t max_function_defs;

  /* Number of IDEFs.  */
  uint16_t max_instruction_defs;

  /* Maximum stack depth across Font Program ('fpgm' table), CVT
     Program ('prep' table) and all glyph instructions (in the 'glyf'
     table).  */
  uint16_t max_stack_elements;

  /* Maximum byte count for glyph instructions.  */
  uint16_t max_size_of_instructions;

  /* Maximum number of components referenced at ``top level'' for any
     composite glyph.  */
  uint16_t max_component_elements;

  /* Maximum levels of recursion; 1 for simple components.  */
  uint16_t max_component_depth;
};

struct sfnt_loca_table_short
{
  /* Offsets to glyph data divided by two.  */
  uint16_t *offsets;

  /* Size of the offsets list.  */
  size_t num_offsets;
};

struct sfnt_loca_table_long
{
  /* Offsets to glyph data.  */
  uint32_t *offsets;

  /* Size of the offsets list.  */
  size_t num_offsets;
};

struct sfnt_glyf_table
{
  /* Size of the glyph data.  */
  size_t size;

  /* Pointer to possibly unaligned glyph data.  */
  unsigned char *glyphs;
};

struct sfnt_simple_glyph
{
  /* The total number of points in this glyph.  */
  size_t number_of_points;

  /* Array containing the last points of each contour.  */
  uint16_t *end_pts_of_contours;

  /* Total number of bytes needed for instructions.  */
  uint16_t instruction_length;

  /* Instruction data.  */
  uint8_t *instructions;

  /* Array of flags.  */
  uint8_t *flags;

  /* Array of X coordinates.  */
  int16_t *x_coordinates;

  /* Array of Y coordinates.  */
  int16_t *y_coordinates;

  /* Pointer to the end of that array.  */
  int16_t *y_coordinates_end;
};

struct sfnt_compound_glyph_component
{
  /* Compound glyph flags.  */
  uint16_t flags;

  /* Component glyph index.  */
  uint16_t glyph_index;

  /* X-offset for component or point number; type depends on bits 0
     and 1 in component flags.  */
  union {
    uint8_t a;
    int8_t b;
    uint16_t c;
    int16_t d;
  } argument1;

  /* Y-offset for component or point number; type depends on bits 0
     and 1 in component flags.  */
  union {
    uint8_t a;
    int8_t b;
    uint16_t c;
    int16_t d;
  } argument2;

  /* Various scale formats.  */
  union {
    uint16_t scale;
    struct {
      uint16_t xscale;
      uint16_t yscale;
    } a;
    struct {
      uint16_t xscale;
      uint16_t scale01;
      uint16_t scale10;
      uint16_t yscale;
    } b;
  } u;
};

struct sfnt_compound_glyph
{
  /* Pointer to array of components.  */
  struct sfnt_compound_glyph_component *components;

  /* Number of elements in that array.  */
  size_t num_components;

  /* Instruction data.  */
  uint8_t *instructions;

  /* Length of instructions.  */
  uint16_t instruction_length;
};

struct sfnt_glyph
{
  /* Number of contours in this glyph.  */
  int16_t number_of_contours;

  /* Coordinate bounds.  */
  sfnt_fword xmin, ymin, xmax, ymax;

  /* Either a simple glyph or a compound glyph, depending on which is
     set.  */
  struct sfnt_simple_glyph *simple;
  struct sfnt_compound_glyph *compound;
};



/* Glyph outline decomposition.  */

struct sfnt_point
{
  /* X and Y in em space.  */
  sfnt_fixed x, y;
};

typedef void (*sfnt_move_to_proc) (struct sfnt_point, void *);
typedef void (*sfnt_line_to_proc) (struct sfnt_point, void *);
typedef void (*sfnt_curve_to_proc) (struct sfnt_point,
				    struct sfnt_point,
				    void *);

typedef struct sfnt_glyph *(*sfnt_get_glyph_proc) (sfnt_glyph, void *,
						   bool *);
typedef void (*sfnt_free_glyph_proc) (struct sfnt_glyph *, void *);



/* Decomposed glyph outline.  */

struct sfnt_glyph_outline_command
{
  /* Flags for this outline command.  */
  int flags;

  /* X and Y position of this command.  */
  sfnt_fixed x, y;
};

/* Structure describing a single recorded outline in fixed pixel
   space.  */

struct sfnt_glyph_outline
{
  /* Array of outlines elements.  */
  struct sfnt_glyph_outline_command *outline;

  /* Size of the outline data, and how much is full.  */
  size_t outline_size, outline_used;

  /* Rectangle defining bounds of the outline.  Namely, the minimum
     and maximum X and Y positions.  */
  sfnt_fixed xmin, ymin, xmax, ymax;

  /* Reference count.  Initially zero.  */
  short refcount;
};

enum sfnt_glyph_outline_flags
  {
    SFNT_GLYPH_OUTLINE_LINETO	  = (1 << 1),
  };



/* Glyph rasterization.  */

struct sfnt_raster
{
  /* Pointer to coverage data.  */
  unsigned char *cells;

  /* Basic dimensions of the raster.  */
  unsigned short width, height;

  /* Integer offset to apply to positions in the raster.  */
  short offx, offy;

  /* The raster stride.  */
  unsigned short stride;

  /* Reference count.  Initially zero.  */
  unsigned short refcount;
};

struct sfnt_edge
{
  /* Next edge in this chain.  */
  struct sfnt_edge *next;

  /* Winding direction.  1 if clockwise, -1 if counterclockwise.  */
  int winding;

  /* X position, top and bottom of edges.  */
  sfnt_fixed x, top, bottom;

  /* step_x is how many pixels to move for each increase in Y by
     SFNT_POLY_STEP.  */
  sfnt_fixed step_x;

#ifdef TEST
  /* Value of x before initial adjustment of bottom to match the
     grid.  */
  sfnt_fixed source_x;
#endif
};



/* Polygon rasterization constants.  */

enum
  {
    SFNT_POLY_SHIFT  = 2,
    SFNT_POLY_SAMPLE = (1 << SFNT_POLY_SHIFT),
    SFNT_POLY_MASK   = (SFNT_POLY_SAMPLE - 1),
    SFNT_POLY_STEP   = (0x10000 >> SFNT_POLY_SHIFT),
    SFNT_POLY_START  = (SFNT_POLY_STEP >> 1),
  };



/* Glyph metrics computation.  */

struct sfnt_long_hor_metric
{
  uint16_t advance_width;
  int16_t left_side_bearing;
};

struct sfnt_hmtx_table
{
  /* Array of horizontal metrics for each glyph.  */
  struct sfnt_long_hor_metric *h_metrics;

  /* Lbearing for remaining glyphs.  */
  int16_t *left_side_bearing;
};

/* Structure describing the metrics of a single glyph.  The fields
   mean the same as in XCharStruct, except they are 16.16 fixed point
   values, and are missing significant information.  */

struct sfnt_glyph_metrics
{
  /* Distance between origin and left edge of raster.  Positive
     changes move rightwards.  */
  sfnt_fixed lbearing;

  /* Advance to next glyph's origin.  */
  sfnt_fixed advance;
};



/* Font style parsing.  */

struct sfnt_name_record
{
  /* Platform identifier code.  */
  uint16_t platform_id;

  /* Platform specific ID.  */
  uint16_t platform_specific_id;

  /* Language identifier.  */
  uint16_t language_id;

  /* Name identifier.  */
  uint16_t name_id;

  /* String length in bytes.  */
  uint16_t length;

  /* Offset from start of storage area.  */
  uint16_t offset;
};

struct sfnt_name_table
{
  /* Format selector of name table.  */
  uint16_t format;

  /* Number of name records.  */
  uint16_t count;

  /* Offset to start of string data.  */
  uint16_t string_offset;

  /* Variable length data.  */
  struct sfnt_name_record *name_records;

  /* Start of string data.  */
  unsigned char *data;
};

/* Name identifier codes.  These are Apple's codes, not
   Microsoft's.  */

enum sfnt_name_identifier_code
  {
    SFNT_NAME_COPYRIGHT_NOTICE			= 0,
    SFNT_NAME_FONT_FAMILY			= 1,
    SFNT_NAME_FONT_SUBFAMILY			= 2,
    SFNT_NAME_UNIQUE_SUBFAMILY_IDENTIFICATION	= 3,
    SFNT_NAME_FULL_NAME				= 4,
    SFNT_NAME_NAME_TABLE_VERSION		= 5,
    SFNT_NAME_POSTSCRIPT_NAME			= 6,
    SFNT_NAME_TRADEMARK_NOTICE			= 7,
    SFNT_NAME_MANUFACTURER_NAME			= 8,
    SFNT_NAME_DESIGNER				= 9,
    SFNT_NAME_DESCRIPTION			= 10,
    SFNT_NAME_FONT_VENDOR_URL			= 11,
    SFNT_NAME_FONT_DESIGNER_URL			= 12,
    SFNT_NAME_LICENSE_DESCRIPTION		= 13,
    SFNT_NAME_LICENSE_INFORMATION_URL		= 14,
    SFNT_NAME_PREFERRED_FAMILY			= 16,
    SFNT_NAME_PREFERRED_SUBFAMILY		= 17,
    SFNT_NAME_COMPATIBLE_FULL			= 18,
    SFNT_NAME_SAMPLE_TEXT			= 19,
    SFNT_NAME_VARIATIONS_POSTSCRIPT_NAME_PREFIX = 25,
  };

struct sfnt_meta_data_map
{
  /* Identifier for the tag.  */
  uint32_t tag;

  /* Offset from start of table to data.  */
  uint32_t data_offset;

  /* Length of the data.  */
  uint32_t data_length;
};

struct sfnt_meta_table
{
  /* Version of the table.  Currently set to 1.  */
  uint32_t version;

  /* Flags.  Currently 0.  */
  uint32_t flags;

  /* Offset from start of table to beginning of variable length
     data.  */
  uint32_t data_offset;

  /* Number of data maps in the table.  */
  uint32_t num_data_maps;

  /* Beginning of variable length data.  */
  struct sfnt_meta_data_map *data_maps;

  /* The whole table contents.  */
  unsigned char *data;
};

enum sfnt_meta_data_tag
  {
    SFNT_META_DATA_TAG_DLNG = 0x646c6e67,
    SFNT_META_DATA_TAG_SLNG = 0x736c6e67,
  };



#define SFNT_CEIL_FIXED(fixed)			\
  (!((fixed) & 0177777) ? (fixed)		\
   : ((fixed) + 0200000) & 037777600000)



/* Function declarations.  Keep these sorted by the order in which
   they appear in sfnt.c.  Keep each line no longer than 80
   columns.  */

#ifndef TEST

extern struct sfnt_offset_subtable *sfnt_read_table_directory (int);

#define PROTOTYPE				\
  int, struct sfnt_offset_subtable *,		\
  struct sfnt_cmap_encoding_subtable **,	\
  struct sfnt_cmap_encoding_subtable_data ***
extern struct sfnt_cmap_table *sfnt_read_cmap_table (PROTOTYPE);
#undef PROTOTYPE

extern sfnt_glyph sfnt_lookup_glyph (sfnt_char,
				     struct sfnt_cmap_encoding_subtable_data *);

#define PROTOTYPE int, struct sfnt_offset_subtable *
extern struct sfnt_head_table *sfnt_read_head_table (PROTOTYPE);
extern struct sfnt_hhea_table *sfnt_read_hhea_table (PROTOTYPE);
extern struct sfnt_loca_table_short *sfnt_read_loca_table_short (PROTOTYPE);
extern struct sfnt_loca_table_long *sfnt_read_loca_table_long (PROTOTYPE);
extern struct sfnt_maxp_table *sfnt_read_maxp_table (PROTOTYPE);
extern struct sfnt_glyf_table *sfnt_read_glyf_table (PROTOTYPE);
#undef PROTOTYPE

extern struct sfnt_glyph *sfnt_read_glyph (sfnt_glyph, struct sfnt_glyf_table *,
					   struct sfnt_loca_table_short *,
					   struct sfnt_loca_table_long *);
extern void sfnt_free_glyph (struct sfnt_glyph *);

#define PROTOTYPE		\
  struct sfnt_glyph *,		\
  struct sfnt_head_table *,	\
  int, sfnt_get_glyph_proc,	\
  sfnt_free_glyph_proc,		\
  void *
extern struct sfnt_glyph_outline *sfnt_build_glyph_outline (PROTOTYPE);
#undef PROTOTYPE

extern void sfnt_prepare_raster (struct sfnt_raster *,
				 struct sfnt_glyph_outline *);

#define PROTOTYPE struct sfnt_glyph_outline *
extern struct sfnt_raster *sfnt_raster_glyph_outline (PROTOTYPE);
#undef PROTOTYPE

#define PROTOTYPE			\
  int,					\
  struct sfnt_offset_subtable *,	\
  struct sfnt_hhea_table *,		\
  struct sfnt_maxp_table *
extern struct sfnt_hmtx_table *sfnt_read_hmtx_table (PROTOTYPE);
#undef PROTOTYPE

extern int sfnt_lookup_glyph_metrics (sfnt_glyph, int,
				      struct sfnt_glyph_metrics *,
				      struct sfnt_hmtx_table *,
				      struct sfnt_hhea_table *,
				      struct sfnt_head_table *,
				      struct sfnt_maxp_table *);

#define PROTOTYPE int, struct sfnt_offset_subtable *
extern struct sfnt_name_table *sfnt_read_name_table (PROTOTYPE);
#undef PROTOTYPE

extern unsigned char *sfnt_find_name (struct sfnt_name_table *,
				      enum sfnt_name_identifier_code,
				      struct sfnt_name_record *);

#define PROTOTYPE int, struct sfnt_offset_subtable *
extern struct sfnt_meta_table *sfnt_read_meta_table (PROTOTYPE);
#undef PROTOTYPE

extern char *sfnt_find_metadata (struct sfnt_meta_table *,
				 enum sfnt_meta_data_tag,
				 struct sfnt_meta_data_map *);

#endif /* TEST */
#endif /* _SFNT_H_ */
