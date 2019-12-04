//
// Created by Zhenyu Zong on 2019/12/4.
//
// Based on C++ library png.h, pngconf.h, pnglibconf.h
//

#ifndef _PNG_H
#define _PNG_H

#include "stdio.h"

#define  FILE_SIGNATURE_SIZE 8

/* Version information for png.h - this should match the version in png.c */
#define PNG_LIBPNG_VER_STRING "1.6.37"
#define PNG_HEADER_VERSION_STRING " libpng version 1.6.37 - April 14, 2019\n"

// Some setting define
#define PNG_LINKAGE_API extern
#define PNGARG(arglist) arglist

#define PNGCAPI
#define PNGCBAPI PNGCAPI
#define PNGAPI PNGCAPI

#define PNG_IMPEXP
#define PNG_FUNCTION(type, name, args, attributes) attributes type name args
#define PNG_EXPORT_TYPE(type) PNG_IMPEXP type

#define PNG_EXPORTA(ordinal, type, name, args, attributes) \
    PNG_FUNCTION(PNG_EXPORT_TYPE(type), (PNGAPI name), PNGARG(args), \
    PNG_LINKAGE_API attributes)

#define PNG_EMPTY /*empty list*/
#define PNG_EXPORT(ordinal, type, name, args) \
    PNG_EXPORTA(ordinal, type, name, args, PNG_EMPTY)
#define PNG_REMOVED(ordinal, type, name, args, attributes)
#define PNG_CALLBACK(type, name, args) type (PNGCBAPI name) PNGARG(args)

#define PNG_ALLOCATED
#define PNG_RESTRICT

/* Basic control structions. Read libpng-manual.txt or libpng.3 for more info.
*
* png_struct is the cache of information used while reading or writing a single
* PNG file.  One of these is always required, although the simplified API
* (below) hides the creation and destruction of it.
*/
typedef struct png_struct_def png_struct;
typedef const png_struct * png_const_structp;
typedef png_struct * png_structp;
typedef png_struct ** png_structpp;

/* png_info contains information read from or to be written to a PNG file.  One
 * or more of these must exist while reading or creating a PNG file.  The
 * information is not used by libpng during read but is used to control what
 * gets written when a PNG file is created.  "png_get_" function calls read
 * information during read and "png_set_" functions calls write information
 * when creating a PNG.
 * been moved into a separate header file that is not accessible to
 * applications.  Read libpng-manual.txt or libpng.3 for more info.
 */
typedef struct png_info_def png_info;
typedef png_info * png_infop;
typedef const png_info * png_const_infop;
typedef png_info ** png_info_pp;

/* Types with names ending 'p' are pointer types.  The corresponding types with
 * names ending 'rp' are identical pointer types except that the pointer is
 * marked 'restrict', which means that it is the only pointer to the object
 * passed to the function.  Applications should not use the 'restrict' types;
 * it is always valid to pass 'p' to a pointer with a function argument of the
 * corresponding 'rp' type.  Different compilers have different rules with
 * regard to type matching in the presence of 'restrict'.  For backward
 * compatibility libpng callbacks never have 'restrict' in their parameters and,
 * consequentially, writing portable application code is extremely difficult if
 * an attempt is made to use 'restrict'.
 */
typedef png_struct * PNG_RESTRICT png_structrp;
typedef const png_struct * PNG_RESTRICT png_const_structrp;
typedef png_info * PNG_RESTRICT png_inforp;
typedef const png_info * PNG_RESTRICT png_const_inforp;

/* Add typedefs for pointers */
typedef unsigned char png_byte;
typedef png_byte              * png_bytep;
typedef const png_byte        * png_const_bytep;
typedef const char            * png_const_charp;
typedef void                  * png_voidp;
typedef png_byte              ** png_bytepp;

typedef FILE                  * png_FILE_p;

typedef PNG_CALLBACK(void, *png_error_ptr, (png_structp, png_const_charp));

// Some typedefs to start
typedef unsigned char png_byte;
typedef unsigned char png_byte;
typedef png_byte * png_bytep;

//#if UINT_MAX > 4294967294U
typedef unsigned int png_uint_32;
//#elif ULONG_MAX > 4294967294U
//typedef unsigned long int png_uint_32;

// Image information
typedef struct ImageInf {
    int bit_depth;
    int color_type;
    unsigned int width;
    unsigned int height;
    unsigned int row_bytes;
    unsigned char** rows;
}ImageInf;

/* Tell lib we have already handled the first <num_bytes> magic bytes.
 * Handling more than 8 bytes from the beginning of the file is an error.
 */
PNG_EXPORT(2, void, png_set_sig_bytes, (png_structrp png_ptr, int num_bytes));

/* Check sig[start] through sig[start + num_to_check - 1] to see if it's a
 * PNG file.  Returns zero if the supplied bytes match the 8-byte PNG
 * signature, and non-zero otherwise.  Having num_to_check == 0 or
 * start > 7 will always fail (ie return non-zero).
 */
PNG_EXPORT(3, int, png_sig_cmp, (png_const_bytep sig, size_t start, size_t num_to_check));

/* Allocate and initialize png_ptr struct for reading, and any other memory. */
PNG_EXPORTA(4, png_structp, png_create_read_struct,
            (png_const_charp user_png_ver, png_voidp error_ptr,
                    png_error_ptr error_fn, png_error_ptr warn_fn),
            PNG_ALLOCATED);

/* Allocate and initialize the info structure */
PNG_EXPORTA(18, png_infop, png_create_info_struct, (png_const_structrp png_ptr),
            PNG_ALLOCATED);

/* Read the information before the actual image data. */
PNG_EXPORT(22, void, png_read_info,
           (png_structrp png_ptr, png_inforp info_ptr));

/* Have the code handle the interlacing.  Returns the number of passes.
 * MUST be called before png_read_update_info or png_start_read_image,
 * otherwise it will not have the desired effect.  Note that it is still
 * necessary to call png_read_row or png_read_rows png_get_image_height
 * times for each pass.
*/
PNG_EXPORT(45, int, png_set_interlace_handling, (png_structrp png_ptr));

/* Optional call to update the users info structure */
PNG_EXPORT(54, void, png_read_update_info, (png_structrp png_ptr,
        png_inforp info_ptr));

/* Read the whole image into memory at once. */
PNG_EXPORT(57, void, png_read_image, (png_structrp png_ptr, png_bytepp image));

/* Initialize the input/output for the PNG file to the default functions. */
PNG_EXPORT(74, void, png_init_io, (png_structrp png_ptr, png_FILE_p fp));

/* Returns number of bytes needed to hold a transformed row. */
PNG_EXPORT(111, size_t, png_get_rowbytes, (png_const_structrp png_ptr,
        png_const_inforp info_ptr));

/* Returns image width in pixels. */
PNG_EXPORT(115, png_uint_32, png_get_image_width, (png_const_structrp png_ptr,
        png_const_inforp info_ptr));

/* Returns image height in pixels. */
PNG_EXPORT(116, png_uint_32, png_get_image_height, (png_const_structrp png_ptr,
        png_const_inforp info_ptr));

/* Returns image bit_depth. */
PNG_EXPORT(117, png_byte, png_get_bit_depth, (png_const_structrp png_ptr,
        png_const_inforp info_ptr));

/* Returns image color_type. */
PNG_EXPORT(118, png_byte, png_get_color_type, (png_const_structrp png_ptr,
        png_const_inforp info_ptr));

// Load png image
ImageInf* png_image_load(char* file_name, ImageInf* image, png_bytep* row_pointers);

#endif //_PNG_H
