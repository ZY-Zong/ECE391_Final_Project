//
// Created by Zhenyu Zong on 2019/12/4.
//

#include "png.h"

// Macro to check failure
#define return_val_if_fail(p, ret) if(!(p)) \
{printf("%s:%d Warning: "#p" failed.\n",\
__func__, __LINE__); return (ret);}

/**
 * Load one png image
 * @param file_name Name of the png image to ba loaded
 * @image pointer to the image, size should be set be the user
 * @return Pointer to information structure of the image
 */
ImageInf* png_image_load(char* file_name, ImageInf* image, png_bytep* row_pointers) {
    int i;

    // Initialize image
    png_byte bit_depth = 0;
    png_byte color_type = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int row_bytes = 0;
    int number_of_passes = 0;

    // Initialize some defined png types
    png_infop info_ptr = NULL;
    png_structp png_ptr = NULL;

    // Open png file
    FILE *fp = fopen(file_name, "rb");
    return_val_if_fail(fp, NULL);

    // Check first 8 bytes of signature
    char signature[FILE_SIGNATURE_SIZE];
    fread(signature, 1, FILE_SIGNATURE_SIZE, fp);
    return_val_if_fail(png_sig_cmp(signature, 0, FILE_SIGNATURE_SIZE) == 0, NULL);

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    return_val_if_fail(png_ptr, NULL);

    info_ptr = png_create_info_struct(png_ptr);
    return_val_if_fail(info_ptr, NULL);

//    return_val_if_fail(setjmp(png_jmpbuf(png_ptr)) == 0, NULL);

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    // Read information before actual image data
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

//    return_val_if_fail(setjmp(png_jmpbuf(png_ptr)) == 0, NULL);

    row_bytes = png_get_rowbytes(png_ptr,info_ptr);

//    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
//    for (i = 0; i < height; i++) {
//        row_pointers[i] = (png_byte*) malloc(row_bytes);
//    }

    // Read image into memory
    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    image->width = width;
    image->height = height;
    image->rows = row_pointers;
    image->row_bytes = row_bytes;
    image->color_type = color_type;
    image->bit_depth = bit_depth;

    return image;
}


