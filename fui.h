#pragma once

//#include "image_fu.h"
//#include "atari-classic.h"

typedef struct {
    int error;

    int x_offset;
    int y_offset;
    int width;
    int height;

    int x;
    int y;
    int padding;

    color_t fg;
    color_t bg;

    image_atlas_t * font;
    image_atlas_t * big_font;

    image_t * output;
} fui_t;

fui_t *
new_fui(image_t * output)
{
    dev_error(output == NULL);

    fui_t * fui = malloc(sizeof(fui_t));
    if (!fui) return NULL;

    fui->output = output;

    fui->x_offset = 0;
    fui->y_offset = 0;
    fui->width = output->width;
    fui->height = output->height;

    fui->fg = GREEN; //rgba_color(0, 240, 0, 255);
    fui->bg = TRANSPARENT;
    fui->font = new_atari_classic(16, fui->fg, fui->bg);
    fui->big_font = new_atari_classic(32, fui->fg, fui->bg);

    fui->padding = 8;

    return fui;
}

int
begin_fui(fui_t * fui)
{
    clear_image(fui->output, 0);
    fui->x = fui->padding;
    fui->y = fui->padding;

    fui->error = 0;

    return 0;
}

int
end_fui(fui_t * fui)
{
    return 0;
}

int
newline_fui(fui_t * fui, int height)
{
    fui->x = fui->padding;
    fui->y = fui->y + height + fui->padding;

    return 0;
}

int
text_fui(fui_t * fui, blob_t * text)
{
    int x = fui->x;
    int y = fui->y;

    flow_image_atlas(fui->font, fui->output, x, y, 0, text);
    newline_fui(fui, fui->font->height);

    return 0;
}

int
text_s64_fui(fui_t * fui, s64 n)
{
    return text_fui(fui, stk_s64_blob(n));
}

int
big_text_fui(fui_t * fui, blob_t * text)
{
    int x = fui->x;
    int y = fui->y;

    flow_image_atlas(fui->big_font, fui->output, x, y, 0, text);
    newline_fui(fui, fui->big_font->height);

    return 0;
}

int
progress_fui(fui_t * fui, float progress)
{
    image_t * img = fui->output;
    color_t fg = fui->fg;

    int x = fui->x;
    int y = fui->y;
    int width = img->width - 2*x;
    int w = width*progress;
    int h = 32;
    for_i(2) {
        draw_rect(img, x + i, y + i, width - 2*i, h - 2*i, &fg);
    }
    fill_rect(img, x, y, w, h, &fg);
//    fill_rect(img, x + w, y, width - w, h, &BLACK);
    newline_fui(fui, h);

    return 0;
}

