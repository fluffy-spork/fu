#ifndef JPEG_FU_H
#define JPEG_FU_H

// out should be width*height*channels size
// TODO(jason): width, height, and channels should be in/out parameters, 0 for whatever is
// in the file.
// need to be able to scale to the output
//rename like bgra_dec_jpeg_fu()?
int
load_jpeg_fu(u8 *out, u8 *in, unsigned long size_in, int *width, int *height, int *channels)
{
    assert(sizeof(u8) == sizeof(JSAMPLE));

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, in, size_in);
    jpeg_read_header(&cinfo, TRUE);
    // these give a different output, I guess u8 single channel
    //cinfo.quantize_colors = 1;
    //cinfo.desired_number_of_colors = 256;
    if (*channels == 4) {
        cinfo.out_color_space = JCS_EXT_BGRA;
    } else if (*channels == 3) {
        cinfo.out_color_space = JCS_YCbCr;
    } else if (*channels == 1) {
        cinfo.out_color_space = JCS_GRAYSCALE;
    }

    if (cinfo.image_width == 320) {
        cinfo.scale_num = 2;
        cinfo.scale_denom = 1;
    }

    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *channels = cinfo.output_components;

    JSAMPROW row[1];

    int stride = cinfo.output_width * cinfo.output_components;

    while (cinfo.output_scanline < cinfo.output_height) {
        row[0] = &out[cinfo.output_scanline*stride];
        jpeg_read_scanlines(&cinfo, row, 1);
    }
    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);

    return 0;
}

int
load_indexed_jpeg_fu(image *indexed, u8 *in, unsigned long size_in, u8 **colormap, int *used_colors)
{
    assert(sizeof(u8) == sizeof(JSAMPLE));
    assert(indexed->channels == 1);

    //debugf("used_colors: %d", *used_colors);

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, in, size_in);
    jpeg_read_header(&cinfo, TRUE);

    cinfo.quantize_colors = true;
    cinfo.dither_mode = JDITHER_NONE;
    // both ignored when colormap supplied
    cinfo.two_pass_quantize = true;
    cinfo.desired_number_of_colors = 128;

    jpeg_start_decompress(&cinfo);

    if (*used_colors > 0) {
        cinfo.actual_number_of_colors = *used_colors;
        cinfo.colormap = colormap;
    }

    JSAMPROW row[1];

    int stride = cinfo.output_width * cinfo.output_components;

    while (cinfo.output_scanline < cinfo.output_height) {
        row[0] = &indexed->data[cinfo.output_scanline*stride];
        jpeg_read_scanlines(&cinfo, row, 1);
    }

    if (*used_colors == 0) {
        *used_colors = cinfo.actual_number_of_colors;
        u8 *red_colormap = colormap[0];
        u8 *green_colormap = colormap[1];
        u8 *blue_colormap = colormap[2];
        for (int i = 0; i < cinfo.actual_number_of_colors; i++) {
            red_colormap[i] = cinfo.colormap[0][i];
            green_colormap[i] = cinfo.colormap[1][i];
            blue_colormap[i] = cinfo.colormap[2][i];
        }
    }

    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);

    return 0;
}

int
_enc_jpeg_fu(u8 *out, unsigned long *size_out, u8 *in, int width, int height, int channels, J_COLOR_SPACE in_color_space)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    //EXTERN(void) jpeg_mem_dest (j_compress_ptr cinfo, unsigned char **outbuffer, unsigned long *outsize);
    // outbuffer pointer to pointer???
    jpeg_mem_dest(&cinfo, &out, size_out);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = channels;
    cinfo.in_color_space = in_color_space;

    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1]; // pointer to a single row 
    // physical row width in buffer
    int row_stride = cinfo.image_width * cinfo.input_components;   // JSAMPLEs per row in image_buffer

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &in[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    *size_out = *size_out - cinfo.dest->free_in_buffer;

    jpeg_finish_compress(&cinfo);

    jpeg_destroy_compress(&cinfo);

    return 0;
}

int
rgb565_enc_jpeg_fu(u8 *out, unsigned long *size_out, u8 *in, int width, int height, int channels)
{
    return _enc_jpeg_fu(out, size_out, in, width, height, channels, JCS_RGB565);
}

int
gray_enc_jpeg_fu(u8 *out, unsigned long *size_out, u8 *in, int width, int height)
{
    return _enc_jpeg_fu(out, size_out, in, width, height, 1, JCS_GRAYSCALE);
}

#endif
