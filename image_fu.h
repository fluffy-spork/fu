#pragma once

#include <limits.h>
#include <math.h>

//#include "asteroids_font.h"
#include "math_fu.h"
#include "srgb2linear_fu.h"

// TODO(jason): this has ended up with too many accidental dependencies
// ^^^ not sure what this meant???  Maybe referring to the headers that had to
// be included.

// TODO(jason): currently, BGRA, but sounds like switching to RGBA is possible.
// Maybe have been due to old SDL.  From google, it looks like RGBA is now
// standard and BGRA is an old hardware thing.
// netbpm P7 PAM format is RGBA

typedef struct {
    int width;
    int height;
    int channels;
    int stride;
    int n_pixels; // TODO(jason): I don't think this is actually useful and possibly encourages bad behavior ignoring stride?
    u8 * data;
} image_t;

#define for_y_image(img) for (int y = 0; y < img->height; y++)
#define for_x_image(img) for (int x = 0; x < img->width; x++)

typedef struct {
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
} color_t;

const color_t WHITE = {
    .red = 255,
    .green = 255,
    .blue = 255,
    .alpha = 255
};

const color_t BLACK = {
    .red = 0,
    .green = 0,
    .blue = 0,
    .alpha = 255
};

const color_t GRAY = {
    .red = 128,
    .green = 128,
    .blue = 128,
    .alpha = 255
};

const color_t RED = {
    .red = 255,
    .green = 0,
    .blue = 0,
    .alpha = 255
};

const color_t ORANGE = {
    .red = 255,
    .green = 128,
    .blue = 0,
    .alpha = 255
};

const color_t GREEN = {
    .red = 0,
    .green = 255,
    .blue = 0,
    .alpha = 255
};

const color_t BLUE = {
    .red = 0,
    .green = 0,
    .blue = 255,
    .alpha = 128
};

const color_t YELLOW = {
    .red = 255,
    .green = 255,
    .blue = 0,
    .alpha = 255
};

// TODO(jason): add size to struct and remove n_pixels
ssize_t
size_image(const image_t * img)
{
    return img->n_pixels * img->channels;
}

size_t
write_image(int fd, image_t * img)
{
    return write(fd, img->data, img->n_pixels * img->channels);
}

bool
equal_image(image_t * a, image_t * b)
{
    if (a->n_pixels  == b->n_pixels
            && a->channels == b->channels
            && (memcmp(a->data, b->data, size_image(a)) == 0)) {
        return true;
    }

    return false;
}

void debug_color(const char *label, const color_t *c)
{
    debugf("%s: rgba %d, %d, %d, %d", label, c->red, c->green, c->blue, c->alpha);
}

// compares the rgb channels, ignores alpha
bool equal_color(const color_t *c1, const color_t *c2)
{
    return c1->red == c2->red
        && c1->green == c2->green
        && c1->blue == c2->blue;
        //&& c1->alpha == c2->alpha;
}

bool is_opaque_color(const color_t *c)
{
    return c->alpha == 255;
}

// https://en.wikipedia.org/wiki/Color_difference
double diff_color(const color_t *c0, const color_t *c1)
{
    float *flut = srgb2linear_f32_fu;
    float Re = flut[c0->red] - flut[c1->red];
    float Ge = flut[c0->green] - flut[c1->green];
    float Be = flut[c0->blue] - flut[c1->blue];

    //int Re = c0->red - c1->red;
    //int Ge = c0->green - c1->green;
    //int Be = c0->blue - c1->blue;
    // closer to 0 is better, coefficients supposedly compensate for human visual perception
    //return sqrt(2.f*Re*Re + 4.f*Ge*Ge + 3.f*Be*Be);
    return sqrt(Re*Re + Ge*Ge + Be*Be);
}

double diff_rgb(const int r, const int g, const int b, const int r1, const int g1, const int b1)
{
    int Re = r - r1;
    int Ge = g - g1;
    int Be = b - b1;
    // closer to 0 is better, coefficients supposedly compensate for human visual perception
    return sqrt(2*Re*Re + 4*Ge*Ge + 3*Be*Be);
}

void
threshold_image(const image_t *in, image_t *out, int threshold, u8 under, u8 over)
{
    assert(in->n_pixels == out->n_pixels);
    assert(in->channels == out->channels);

    int imax = in->n_pixels * in->channels;
    for (int i = 0; i < imax; i++) {
        out->data[i] = in->data[i] < threshold ? under : over;
    }
}

int
percent_diff_image(const image_t *a, const image_t *b, image_t *c, int threshold)
{
    assert(a->n_pixels == b->n_pixels);
    assert(a->channels == b->channels);

    u32 total = 0;
    u32 diff = 0;

    int max = a->n_pixels * a->channels;
    for (int i = 0; i < max; i++) {
        total++;
        //if (abs(a->data[i] - b->data[i]) > 0) {
            //debugf("diff: a: %d, b: %d", a->data[i], b->data[i]);
        //}

        if (abs(a->data[i] - b->data[i]) > threshold) {
            c->data[i] = a->data[i];
            diff += 100;
        } else {
            c->data[i] = 0;
        }
    }

    return diff/total;
}

int
sub_image(image_t *a, image_t *b, image_t *c, int threshold)
{
    assert(a->n_pixels == b->n_pixels);
    assert(b->n_pixels == c->n_pixels);
    assert(a->channels == b->channels);
    assert(b->channels == c->channels);

    int max = a->n_pixels * a->channels;

    for (int i = 0; i < max; i += 4) {
        //double diff = diff_rgb(a->data[i + 2], a->data[i + 1], a->data[i], b->data[i + 2], b->data[i + 1], b->data[i]);
        //debugf("diff: %f", diff);
        if (abs(a->data[i] - b->data[i]) < threshold
                && abs(a->data[i + 1] - b->data[i + 1]) < threshold
                && abs(a->data[i + 2] - b->data[i + 2]) < threshold)
        {
        //if (diff < threshold) {
            c->data[i] = 0;
            c->data[i + 1] = 255;
            c->data[i + 2] = 0;
            c->data[i + 3] = 255;
        } else {
            c->data[i] = a->data[i];
            c->data[i + 1] = a->data[i + 1];
            c->data[i + 2] = a->data[i + 2];
            c->data[i + 3] = a->data[i + 3];
        }
    }

    return 0;
}

int
min(int a, int b)
{
    return a < b ? a : b;
}

int
max(int a, int b)
{
    return a > b ? a : b;
}

// https://en.wikipedia.org/wiki/Insertion_sort
void
insertion_sort_u8(u8 *a, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        u8 x = a[i];
        int j = i - 1;
        for (; j >=0 && a[j] > x; j--) {
            a[j + 1] = a[j];
        }
        a[j + 1] = x;
    }
}

void
test_insertion_sort_u8()
{
    u8 a[] = { 33, 12, 9, 3, 42, 88 };
    int n = sizeof(a)/sizeof(a[0]);

    insertion_sort_u8(a, n);

    printf("XXXXXXXX testing insertion sort\n");
    for (int i = 0; i < n; i++) {
        printf("%u, ", a[i]);
    }
    printf("\n");
}

void
median_channel(image_t *in, image_t *out, const int r, const int channel)
{
    assert(in->n_pixels == out->n_pixels);
    assert(in->channels == out->channels);

    //u64 start = SDL_GetPerformanceCounter();

    //const int r = 1;
    const int n = (2*r + 1)*(2*r + 1);
    //const int pad = n/2;
    u8 win[n];
    const int median = n/2;
    const int pad = r*in->stride + r*in->channels + channel;
    //const int imax = (a->n_pixels - r) * a->channels - r*a->stride;
    const int imax = in->n_pixels * in->channels - pad;

    memcpy(out->data, in->data, pad);
    memcpy(out->data + imax, in->data + imax, pad);

    // XXX(jason): treating this as one long 1D array causes problems on the
    // edge when left and right are different
    for (int i = pad; i < imax; i += in->channels) {
        for (int j = 0, k = -r; k <= r; k++) {
            int x = i + k*in->channels;
            for (int y = -r; y <= r; y++) {
                win[j++] = in->data[x + y*in->stride];
            }
        }

        insertion_sort_u8(win, n);

        out->data[i] = win[median];
    }
    //debugf("median_channel: %ld", (SDL_GetPerformanceCounter() - start)/1000);
}

void
median_image(image_t *in, image_t *out, int radius)
{
    assert(in != out);

    struct timespec time;
    elapsed_debug(&time, NULL, NULL);

    median_channel(in, out, radius, 0);

    if (in->channels > 1) {
        median_channel(in, out, radius, 1);
    }

    if (in->channels > 2) {
        median_channel(in, out, radius, 2);
    }

    // ignore assumed alpha 4th channel

    struct timespec elapsed;
    elapsed_debug(&time, &elapsed, "media_image");
}

void
quantize_image(const image_t *in, image_t *out, int colors_per_channel)
{
    assert(colors_per_channel > 0);
    assert(colors_per_channel <= 256);

    int m = 256/colors_per_channel;

    // seems like this would be better if it rounded instead of truncate
    // update: it's not.
    const int max = in->n_pixels * in->channels;
    for (int i = 0; i < max; i++) {
        out->data[i] = (in->data[i] / m) * m;
    }
}

int get_yuyv_pixel(u8 *yuyv, int stride, int x, int y, color_t *pixel)
{
    int i = y*stride + x*2;

    int Y = yuyv[i];
    int U, V;
    if (i % 4 == 2) {
        U = yuyv[i - 1];
        V = yuyv[i + 1];
    } else {
        U = yuyv[i + 1];
        V = yuyv[i + 3];
    }

    pixel->red = clamp255(round(Y + 1.402*(V - 128)));
    pixel->green = clamp255(round(Y - 0.344*(U - 128) - 0.714*(V - 128)));
    pixel->blue = clamp255(round(Y + 1.722*(U - 128)));
    pixel->alpha = 255;

    //debugf("pixel: %d %d %d", pixel->red, pixel->green, pixel->blue);

    return 0;
}

/* distance between 2 points */
double distance(double x0, double y0, double x1, double y1)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    return sqrt(dx*dx + dy*dy);
}

u8
get_gray(const image_t * img, int x, int y)
{
    return img->data[y*img->stride + x];
}

u8
set_gray(const image_t * img, int x, int y, u8 value)
{
    return img->data[y*img->stride + x] = value;
}

int get_pixel(const image_t *img, int x, int y, color_t *pixel)
{
    assert(img->channels == 4);

    int i = y*img->width*img->channels + x*img->channels;
    pixel->blue = img->data[i];
    pixel->green = img->data[i + 1];
    pixel->red = img->data[i + 2];
    pixel->alpha = img->data[i + 3];

    return 0;
}

void get_pixel_at(const image_t *img, int p, color_t *pixel)
{
    int i = p*img->channels;

    pixel->blue = img->data[i];
    pixel->green = img->data[i + 1];
    pixel->red = img->data[i + 2];
    pixel->alpha = img->data[i + 3];
}

void
set_pixel_at(image_t *img, int p, color_t *pixel)
{
    int i = p*img->channels;

    img->data[i] = pixel->blue;
    img->data[i + 1] = pixel->green;
    img->data[i + 2] = pixel->red;
    img->data[i + 3] = pixel->alpha;
}

void
set_pixel(image_t *img, int x, int y, const color_t *fg)
{
    assert(x >= 0);
    assert(x < img->width);
    assert(y >= 0);
    assert(y < img->height);

    int i = y*img->width*img->channels + x*img->channels;
    u8 *data = img->data;

    if (img->channels == 4) {
        data[i] = fg->blue;
        data[i + 1] = fg->green;
        data[i + 2] = fg->red;
        data[i + 3] = fg->alpha;
    }
    if (img->channels == 3) {
        data[i] = fg->red;
        data[i + 1] = fg->green;
        data[i + 2] = fg->blue;
    } else {
        data[i] = fg->blue;
    }
}

image_t *
new_image(int width, int height, int channels)
{
    image_t *img = malloc(sizeof(*img));
    img->n_pixels = height*width;
    img->height = height;
    img->width = width;
    img->channels = channels;
    img->stride = img->width*channels; // 1 byte per channel per pixel
    // doesn't seem like this should be zero'd
    img->data = calloc(img->n_pixels*channels, sizeof(img->data[0]));

    return img;
}

// Wrap an image struct around an existing data array.  Your job to manage
// freeing data appropriately.
// could there be a static_image macro that creates a static array and wraps it 
void
wrap_image(image_t * img, u8 * data, int width, int height, int channels)
{
    img->data = data;
    img->n_pixels = height*width;
    img->height = height;
    img->width = width;
    img->channels = channels;
    img->stride = img->width*channels;
}

/*
int
view_image(image_t * view, const image_t * src, int x, int y)
{
    view->data = src->data[y*src->stride + x];
    view->channels = src->channels;
    view->stride = src->stride;

    return 0;
}
*/

image_t *
like_image(const image_t *in)
{
    return new_image(in->width, in->height, in->channels);
}

// YV12 is the MS recommended video YUV420 video format.
// This is causing grayscale images to be too dark when displayed with SDL.
// IDK what the problem is, but copying to an RGBA image and displaying is
// brighter.  I assume maybe it's a gamma issue or something, but I want to
// replace it all with floats anyway so putting it off until later.  Currently
// just convert gray to rgba when copying to final image.
image_t *
new_yv12_image(int width, int height)
{
    image_t *img = malloc(sizeof(*img));
    img->n_pixels = width*height;
    img->height = height;
    img->width = width;
    img->channels = 1;
    img->stride = img->width;
    img->data = calloc(img->n_pixels + img->n_pixels/2, sizeof(img->data[0]));

    memset(img->data, 0x80, img->n_pixels + img->n_pixels/2);

    return img;
}

void free_image(image_t *img)
{
    if (img) {
        free(img->data);
        free(img);
    }
}

/* are the pixel coordinates inside the image bounds */
bool
pixel_in_image(image_t *img, int x, int y)
{
    return x >= 0 && x < img->width && y >= 0 && y < img->height;
}


void
info_image(const image_t *in, const char *label)
{
    debugf("%s: %dx%dx%d", label, in->width, in->height, in->channels);
}

void clear_image(image_t *img, int value)
{
    memset(img->data, value, img->n_pixels*img->channels);
}

void
plane_image(const image_t * img, int channel, u8 * buf, size_t size_buf)
{
    size_t size = size_image(img);
    for (int i = channel, j = 0; i < size; i += 4, j++) {
        buf[j] = img->data[i];
    }
}


#define debug_xy_image(img, max, x, y) _debug_image(img, max, x, y)
#define debug_image(img, max) debug_xy_image(img, max, 0, 0)

void
_debug_image(image_t *img, size_t max, int x_off, int y_off)
{
    size_t y_max = min_size(img->height, y_off + max);
    size_t x_max = min_size(img->width, x_off + max);

    for (size_t y = y_off; y < y_max; y++) {
        for (size_t x = x_off; x < x_max; x++) {
            debugf("[%zdx%zd] = %u", x, y, img->data[y*img->width + x]);
        }
        debug("");
    }
}

void copy_image(const image_t *src, image_t *dest)
{
    assert(src != dest);
    assert(src->n_pixels == dest->n_pixels);
    assert(src->channels == dest->channels);

    memcpy(dest->data, src->data, src->n_pixels*src->channels);
}

void
copy_rect_image(int width, int height, const image_t *src, int x1, int y1, image_t *dest, int x2, int y2)
{
    //assert(src->channels == 1);
    assert(src->channels == dest->channels);
    assert(x2 < dest->width);
    assert(y2 < dest->height);
    assert(x1 < src->width);
    assert(y1 < src->height);
    assert(width + x2 <= dest->width);
    assert(height + y2 <= dest->height);

    if (src->channels == 1) {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                dest->data[(y2 + i)*dest->width + x2 + j] = src->data[(y1 + i)*src->width + x1 + j];
            }
        }
    } else if (src->channels == 4) {
        u32 *sdata = (u32 *)src->data;
        u32 *ddata = (u32 *)dest->data;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int di = (y2 + i)*dest->width + x2 + j;
                int si = (y1 + i)*src->width + x1 + j;
                ddata[di] = sdata[si];
            }
        }
    }
}

void
paste_image(image_t *src, image_t *dest, int x, int y)
{
    copy_rect_image(src->width, src->height, src, 0, 0, dest, x, y);
}

void
paste_center_image(image_t * src, image_t * dest)
{
    paste_image(src, dest, abs(dest->width - src->width)/2, abs(dest->height - src->height)/2); 
}

// copy a region of interest from larger image
void
roi_image(image_t *roi, const image_t *img, int x, int y)
{
    x = clamp(x, 0, img->width - 1);
    y = clamp(y, 0, img->height - 1);
    copy_rect_image(roi->width, roi->height, img, x, y, roi, 0, 0);
}

// flut is size 256
void
flut_image(image_t *img, float *flut)
{
    size_t imax = img->n_pixels * img->channels;
    for (size_t i = 0; i < imax; i++) {
        float v = flut[img->data[i]];
        img->data[i] = floor(v*255);
    }
}

void
lut_image(const image_t *in, image_t *out, const u8 *lut)
{
    //assert(in->channels == 4);
    assert(in->channels == out->channels);

    // TODO(jason): what does this mean?  how would alpha become 255?
    // alpha is typically 255 and so converts to 255
    size_t imax = in->n_pixels * in->channels;
    for (size_t i = 0; i < imax; i++) {
        out->data[i] = lut[in->data[i]];
    }
}

void
lut_channel_image(const image_t *in, image_t *out, const u8 *lut, int channel)
{
    assert(channel < in->channels);
    assert(in->channels == out->channels);
    assert(in->n_pixels == out->n_pixels);

    // alpha is typically 255 and so converts to 255
    size_t imax = in->n_pixels * in->channels;
    for (size_t i = channel; i < imax; i += in->channels) {
        out->data[i] = lut[in->data[i]];
    }
}

void
lut_gray_image(const image_t * in, image_t * out, const u8 * lut)
{
    assert(in->channels == 1);

    lut_channel_image(in, out, lut, 0);
}

// add a constant value to every pixel component clamped to 0, 255
void
add_image(image_t *img, const u8 value)
{
    // alpha is typically 255 and so converts to 255
    size_t imax = img->n_pixels * img->channels;
    for (size_t i = 0; i < imax; i++) {
        img->data[i] = clamp((int)img->data[i] + value, 0, 255);
    }
}

float 
diff_ratio_color(const color_t *c1, const color_t *c2)
{
    // the flut could possibly be modified to limit/quantize colors
    float *flut = srgb2linear_f32_fu;
    float r1 = flut[c1->red];
    float g1 = flut[c1->green];
    float b1 = flut[c1->blue];

    float r2 = flut[c2->red];
    float g2 = flut[c2->green];
    float b2 = flut[c2->blue];

    // handle dark gray road with white and yellow lines
    // white/gray is 1/3, 1/3, 1/3
    // black will div by 0
    float s1 = r1 + g1 + b1;
    float s2 = r2 + g2 + b2;

    //debugf("s1: %f, s2: %f", s1, s2);
    // ???  this may be too low.
    if (s1 < 0.005f && s2 < 0.005f) {
        // both effectively black.  small values can have non-black ratios when the
        // overall luminance is low enough to be black.  like r:g:b 5:1:0 is
        // black but largely red in ratio.
        return 0.0f;
    }

    // effectively white seems like it should be a thing too

    // if only one was black, can't have divide by 0.  possibly should just
    // force the srgb2linear 0 value to be non-zero.
    if (s1 < 0.0001f) s1 = 0.0001f;
    if (s2 < 0.0001f) s2 = 0.0001f;

    // SAD
    float dr = fabsf(r1/s1 - r2/s2);
    float dg = fabsf(g1/s1 - g2/s2);
    float db = fabsf(b1/s1 - b2/s2);

    return dr + dg + db;
}

// sum of absolute differences in rgb
int
sad_color(const color_t *c1, const color_t *c2)
{
    // TODO(jason): right now full mapping vision region srgb2linear
    /*
    return abs(u8_srgb2linear[c1->red] - u8_srgb2linear[c2->red])
        + abs(u8_srgb2linear[c1->green] - u8_srgb2linear[c2->green])
        + abs(u8_srgb2linear[c1->blue] - u8_srgb2linear[c2->blue]);
        */
    return abs(c1->red - c2->red)
        + abs(c1->green - c2->green)
        + abs(c1->blue - c2->blue);
}

u64
sad_image(image_t *img1, image_t *img2)
{
    assert(img1->width == img2->width);
    assert(img1->height == img2->height);

    u64 sad = 0;
    size_t imax = img1->n_pixels * img1->channels;
    for (size_t i = 0; i < imax; i++) {
        sad += abs((int)img1->data[i] - img2->data[i]);
    }

    return sad;
}

#define debug_int_array(a, n) debug_array(a, n, #a)

void
debug_array(int a[], int n, char *label)
{
    if (label) {
        printf("\n%s: ", label);
    }

    for (int i = 0; i < n; i++) {
        printf("%d:%d\n", i, a[i]);
    }
    printf("\n");
}

void
debug_u8_array(u8 a[], int n, char *label)
{
    if (label) {
        printf("\n%s: ", label);
    }

    for (int i = 0; i < n; i++) {
        printf("%d: %u\n", i, a[i]);
    }
    printf("\n");
}

void
lut_u8(u8 *in, u8 *out, size_t n, u8 *lut)
{
    for (size_t i = 0; i < n; i++) {
        out[i] = lut[in[i]];
    }
}

// build a lut based on historgram equalization
void
histogram_equalization_lut(const u8 *in, size_t n, int channels, int ch, u8 *lut)
{
    assert(n > 0);
    assert(n < INT_MAX);

    size_t imax = n * channels;

    int hist[256] = {};
    for (size_t i = ch; i < imax; i += channels) {
        hist[in[i]]++;
    }

    //do something with min and max percentage off beginning and end of hist

    int cdf[256] = {};
    float min_cdf = 0.f;
    int total = 0;
    size_t offset = 0;
    for (size_t i = offset; i < 256 - offset; i++) {
        total += hist[i];
        if (min_cdf == 0.f && total > 0) {
            // first non-zero value is min
            min_cdf = hist[i];
        }
        cdf[i] = total;
    }

    float den = n - min_cdf;
    for (size_t i = 0; i < 256; i++) {
        if (cdf[i] > 0) {
            lut[i] = roundf((cdf[i] - min_cdf)/den * 255.f);
        } else {
            lut[i] = 0;
        }
    }
}

void
histogram_equalization_image(const image_t *in, image_t *out)
{
    assert(in->channels == 4 || in->channels == 1);

    if (in->channels == 4) {
        // these may be out of order, but it doesn't really matter as long as
        // they're used in the same order
        u8 red_lut[256];
        u8 green_lut[256];
        u8 blue_lut[256];

        histogram_equalization_lut(in->data, in->n_pixels, in->channels, 0, red_lut);
        histogram_equalization_lut(in->data, in->n_pixels, in->channels, 1, green_lut);
        histogram_equalization_lut(in->data, in->n_pixels, in->channels, 2, blue_lut);

        int imax = in->n_pixels * in->channels;
        for (int i = 0; i < imax; i += in->channels) {
            out->data[i] = red_lut[in->data[i]];
            out->data[i + 1] = green_lut[in->data[i + 1]];
            out->data[i + 2] = blue_lut[in->data[i + 2]];
            out->data[i + 3] = in->data[i + 3];
        }
    } else if (in->channels == 1) {
        u8 lut[256];
        histogram_equalization_lut(in->data, in->n_pixels, in->channels, 0, lut);
        lut_channel_image(in, out, lut, 0);
    }
}

int
test_histogram_equalization_image()
{
    size_t size_test = 64;
    u8 in_test[64] = {
        52, 55, 61, 59, 70, 61, 76, 61,
        62, 59, 55, 104, 94, 85, 59, 71,
        63, 65, 66, 113, 144, 104, 63, 72,
        64, 70, 70, 126, 154, 109, 71, 69,
        67, 73, 68, 106, 122, 88, 68, 68,
        68, 79, 60, 79, 77, 66, 58, 75,
        69, 85, 64, 58, 55, 61, 65, 83,
        70, 87, 69, 68, 65, 73, 78, 90
    };
    u8 out_test[64];
    u8 test_out_test[64] = {
        0, 12, 53, 32, 146, 53, 174, 53,
        57, 32, 12, 227, 219, 202, 32, 154,
        65, 85, 93, 239, 251, 227, 65, 158,
        73, 146, 146, 247, 255, 235, 154, 130,
        97, 166, 117, 231, 243, 210, 117, 117,
        117, 190, 36, 190, 178, 93, 20, 170,
        130, 202, 73, 20, 12, 53, 85, 194,
        146, 206, 130, 117, 85, 166, 182, 215
    };

    u8 he_lut[256];
    histogram_equalization_lut(in_test, size_test, 1, 0, he_lut);
    lut_u8(in_test, out_test, size_test, he_lut);

    int errors = 0;
    for (size_t i = 0; i < size_test; i++) {
        if (out_test[i] != test_out_test[i]) {
            debugf("invalid: [%zd] %d != %d", i, out_test[i], test_out_test[i]);
            errors ++;
        }
    }

    return errors;
}

int
normalize_image(const image_t *in, image_t *out)
{
    assert(in->channels == 4);
    assert(in->n_pixels == out->n_pixels);

    // calculate Y and color ratios for each pixel
    // find the min and max
    // calculate new Y and then RGB

    size_t n = in->n_pixels;

    u8 *luma = malloc(in->n_pixels*sizeof(u8));
    if (!luma) return ENOMEM;

    color_t pixel;
    for (size_t i = 0; i < n; i++)
    {
        get_pixel_at(in, i, &pixel);

        luma[i] = roundf(0.212671f*pixel.red + 0.715160f*pixel.green + 0.072169f*pixel.blue);
    }

    u8 luma_lut[256];

    histogram_equalization_lut(luma, n, 1, 0, luma_lut);

    //debug_u8_array(luma_lut, 256, "luma_lut");

    for (size_t i = 0; i < n; i++)
    {
        u8 luma2 = luma_lut[luma[i]];
        pixel.red = luma2;
        pixel.green = luma2;
        pixel.blue = luma2;
        pixel.alpha = 255; // don't really care

        set_pixel_at(out, i, &pixel);
    }

    free(luma);

    return 0;
}

int
lerp_image(const image_t *in, image_t *out)
{
    assert(in->channels == 1);
    assert(in->channels == out->channels);

    u8 min = 255;
    u8 max = 0;

    for (int i = 0; i < in->n_pixels; i++) {
        u8 v = in->data[i];
        if (v < min) min = v;
        if (v > max) max = v;
    }

    int range = max - min;
    for (int i = 0; i < in->n_pixels; i++) {
        out->data[i] = ((in->data[i] - min) * 255)/range;
    }

    return 0;
}

void
and_binary_image(const image_t *a, const image_t *b, image_t *c)
{
    assert(a->channels == 1);
    assert(a->channels == b->channels);
    assert(b->channels == c->channels);
    assert(a->n_pixels == b->n_pixels);
    assert(b->n_pixels == c->n_pixels);

    for (int i = 0; i < a->n_pixels; i++) {
        c->data[i] = (a->data[i] && b->data[i]) ? 255 : 0;
    }
}

void
or_binary_image(const image_t *a, const image_t *b, image_t *c)
{
    assert(a->channels == 1);
    assert(a->channels == b->channels);
    assert(b->channels == c->channels);
    assert(a->n_pixels == b->n_pixels);
    assert(b->n_pixels == c->n_pixels);

    for (int i = 0; i < a->n_pixels; i++) {
        c->data[i] = (a->data[i] || b->data[i]) ? 255 : 0;
    }
}

int
median_hist(int hist[], int n, int max)
{
    int target = (max + 1)/2;
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += hist[i];
        if (total > target) {
            return i;
        }
    }

    return -1;
}

u64 sum_array(int a[], int n)
{
    assert(n <= 256);

    u64 sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i];
    }

    return sum;
}

int
index_of_max_array(const int a[], const int n)
{
    int vmax = 0;
    int imax = 0;
    for (int i = 0; i < n; i++) {
        int v = a[i];
        // later equal values won't override existing max and index?  Maybe
        // should have a parameter for how ties should be broken.  This will
        // bias towards darker colors for histograms.
        if (v > vmax) {
            vmax = v;
            imax = i;
        }
    }

    return imax;
}

int
probably_unimodal_histogram(int hist[], int n)
{
    int threshold = hist[index_of_max_array(hist, n)]/8;

    //this needs to work to identify single peak
    int peaks = 0;
    int high = 0;
    for (int i = 0; i < n; i++) {
        if (hist[i] > threshold) {
            high = 1;
        } else if (hist[i] < threshold) {
            if (high) peaks++;
            high = 0;
        }
    }

    debugf("peaks: %d", peaks);
    return peaks <= 1;
}

int
otsu_histogram_threshold(int hist[], int n)
{
    //if (probably_unimodal_histogram(hist, n)) return 0;

    u64 total = sum_array(hist, n);
    //debugf("total: %ld, peaks: %d", total, count_peaks_histogram(hist, n));

    u64 sumB = 0;
    u64 wB = 0;
    // XXX(jason): due to compiler error about uninitialized usage setting this
    // to 0.0, but not sure what it should be.  probably need to just delete
    // this function anyway.
    double max = 0.0;

    int level = 0;

    u64 sum1 = 0;
    for (int i = 0; i < n; i++) {
        sum1 += i * hist[i];
    }

    for (int i = 0; i < n; i++) {
        wB += hist[i];
        u64 wF = total - wB;
        if (wB == 0 || wF == 0) {
            continue;
        }

        sumB += i * hist[i];
        u64 mF = (sum1 - sumB)/wF;
        double between = wB * wF * ((sumB / wB) - mF) * ((sumB / wB) - mF);
        if (between >= max) {
            level = i;
            max = between;
        }
    }

    return level;
}

int
weigh_histogram(int offset, int n, int hist[])
{
    int weight = 0;
    for (int i = offset; i < n; i++)
    {
        weight += hist[i];
    }
    return weight;
}

int
balanced_histogram_threshold(int hist[], int n)
{
    int start = 0;
    int end = n - 1;
    int mid = (start + end)/2;
    int left_weight = weigh_histogram(start, mid + 1, hist);
    int right_weight = weigh_histogram(mid + 1, end + 1, hist);
    //debugf("lw: %d, rw: %d", left_weight, right_weight);
    if (left_weight == 0 && right_weight == 0) return -1;
    while (start <= end)
    {
        if (right_weight > left_weight)
        {
            right_weight -= hist[end];
            end--;
            if (((start + end)/2) < mid)
            {
                right_weight -= hist[mid];
                left_weight += hist[mid];
                mid++;
            }
        }
        else {
            left_weight -= hist[start];
            start++;
            if (((start + end)/2) >= mid)
            {
                left_weight -= hist[mid + 1];
                right_weight += hist[mid + 1];
                mid--;
            }
        }
    }

    //debugf("mid: %d", mid);
    return mid;
}

/*
   typedef struct rg_chromaticity
{
    u8 r;
    u8 g;
    u16 G;
} rg_chromaticity;

rg_chromaticity
rgb_to_rg_chromaticity(u8 R, u8 G, u8 B)
{
    rg_chromaticity rg;
    int s = R + G + B;
    if (s) {
        rg.r = ((int)R * 255)/s;
        rg.g = ((int)G * 255)/s;
    } else {
        rg.r = 85;
        rg.g = 85;
    }
    rg.G = s;

    return rg;
    //histogram equalization on average-rgb intensity for each chromaticity to
    //find high and low shades of colors
    //median_index_image with rg_chromaticity
}

color_t
rg_chromaticity_to_color(rg_chromaticity *rg)
{
    color_t rgb;
    //rgb.green = rg->G;
    if (rg->G) {
        rgb.red = (rg->G*rg->r)/255;
        rgb.green = (rg->G*rg->g)/255;
        rgb.blue = (rg->G*(765 - rg->r - rg->g))/255;
        //rgb.red = ((int)rg->r*rg->G)/rg->g;
        //rgb.blue = ((765 - rg->r - rg->g)*rg->G)/rg->g;
    } else {
        rgb.red = 0;
        rgb.blue = 0;
    }

    return rgb;
}

void
debug_rg_chromaticity(rg_chromaticity *rg)
{
    debugf("rg_chromaticity r %u, g %u, G %u", rg->r, rg->g, rg->G);
}
*/

typedef struct
{
    float h;
    float s;
    float v;
} hsv_fu;

void
color_to_hsi(const color_t *srgb, float *h, float *s, float *i)
{
    float r = srgb->red/255.f;
    float g = srgb->green/255.f;
    float b = srgb->blue/255.f;
    float M = fmaxf(fmaxf(r, g), b);
    float m = fminf(fminf(r, g), b);
    float C = M - m;

    //debugf("M: %f, m: %f, C: %f", M, m, C);

    *i = (r + g + b)/3;
    *s = (*i > 0.f) ? 1 - m/(*i) : 0.f;
    if (C == 0.f) {
        *h = 0.f;
    } else if (M == r) {
        *h = (g - b)/C;
    } else if (M == g) {
        *h = (b - r)/C + 2.f;
    } else if (M == b) {
        *h = (r - g)/C + 4.f;
    }

    *h *= 60.f; // /360.f;
    if (*h < 0.f) {
        *h = *h + 360.f;
    }
    *h /= 360.f;
}

typedef struct chromaticity
{
    float Y; // luminance
    float x;
    float y;
    float z;
} chromaticity;

// color is assumed sRGB
void
color_to_chromaticity(const color_t *srgb, chromaticity *chr)
{
    float *flut = srgb2linear_f32_fu;
    float R = flut[srgb->red];
    float G = flut[srgb->green];
    float B = flut[srgb->blue];

    float X = 0.412453f*R + 0.357580f*G + 0.180423f*B;
    float Y = 0.212671f*R + 0.715160f*G + 0.072169f*B;
    float Z = 0.019334f*R + 0.119193f*G + 0.950227f*B;

    float XYZ = X + Y + Z;

    if (XYZ > 0.f) {
        chr->Y = Y;
        chr->x = X/XYZ;
        chr->y = Y/XYZ;
    } else { // black chromaticity is same as white
        chr->Y = 0.f;
        chr->x = 0.312731f;
        chr->y = 0.329033f;
    }
}

void
color_to_ratio(const color_t *srgb, chromaticity *chr)
{
    float R = srgb->red/255.f;
    float G = srgb->green/255.f;
    float B = srgb->blue/255.f;

    //float Y = 0.212671f*R + 0.715160f*G + 0.072169f*B;
    float Y = 0.2989f*R + 0.5870f*G + 0.1140f*B;
    float RGB = R + G + B;

    if (RGB > 0.f) {
        chr->Y = Y;
        chr->x = R/RGB;
        chr->y = G/RGB;
        chr->z = B/RGB;
    } else { // black chromaticity is same as white
        chr->Y = 0.f;
        chr->x = 0.333333f;
        chr->y = 0.333333f;
        chr->z = 0.333333f;
    }
}

void
debug_chromaticity(int i, const chromaticity *chroma)
{
    debugf("chromaticity[%d]: x %f y %f Y %f", i, chroma->x, chroma->y, chroma->Y);
}

void
histogram_image(const image_t *img, size_t channel, int hist[], size_t size_hist)
{
    memset(hist, 0, sizeof(int)*size_hist);

    size_t imax = img->n_pixels * img->channels;
    for (size_t i = channel; i < imax; i += img->channels) {
        hist[img->data[i] % size_hist]++;
    }
}

// find the minimum index with the summed values at least target_sum 
size_t
min_sum_index_histogram(const int * hist, size_t size_hist, s64 target_sum)
{
    s64 sum = 0;

    for (size_t i = 0; i < size_hist; i++) {
        sum += hist[i];
        if (sum >= target_sum) return i;
    }

    return 255;
}

size_t
first_nonzero_index_histogram(const int * hist, size_t size_hist, size_t default_index)
{
    for (size_t i = 0; i < size_hist; i++) {
        if (hist[i]) return i;
    }

    return default_index;
}


// count how many values are non-zero
u64
count_histogram_image(int hist[], size_t size_hist, size_t offset)
{
    u64 count = 0;
    for (size_t i = offset; i < size_hist; i++) {
        if (hist[i]) count++;
    }

    return count;
}

int
mean_histogram(int hist[], int size_hist)
{
    assert(size_hist <= 256);

    u64 sum = 0;
    u64 total = 0;
    for (int i = 0; i < size_hist; i++)
    {
        total += hist[i];
        sum += i*hist[i];
    }

    if (total > 0) {
       return sum/total;
    } else {
       return -1;
    }
}

// index_bg is an index to ignore (background) when finding max
int
max_index_image(const image_t *in, int index_bg)
{
    int hist[256];
    //copy_rect_image(box->width, box->height, in, x*x_stride, y*y_stride, box, 0, 0);
    histogram_image(in, 0, hist, 256);
    if (index_bg >= 0 && index_bg < 256) hist[index_bg] = 0;
    return index_of_max_array(hist, 256);
}

// n is number of base colors in palette.  2 sets of colors is palette 1 light
// and 1 dark
/*
int
median_index_image(const image *in, const color *palette, const int n_palette)
{
    assert(n_palette <= 256);

    int hist[256] = {};

    chromaticity chroma_palette[256];
    for (int i = 0; i < n_palette; i++)
    {
        color_to_chromaticity(&palette[i], &chroma_palette[i]);
        //debugf("chroma palette[%d]: %f, %f, %f", i, chroma_palette[i].Y, chroma_palette[i].x, chroma_palette[i].y);
    }

    //const int shade_offset = (n_palette - 2)/2;

    //regardless of if chromaticity is used, the Y still needs to be normalized per color

    chromaticity chroma;
    color_t pixel;
    for (int y = 0; y < in->height; y++) {
        for (int x = 0; x < in->width; x++) {
            get_pixel(in, x, y, &pixel);

            color_to_chromaticity(&pixel, &chroma);
            //debugf("chroma.Y: %f", chroma.Y);

            if (chroma.Y < 0.25f) { // Y for dark blue (rgb = 0, 0, 128) is only 0.002295
                // black
                hist[0]++;
            } else if (chroma.Y > 0.75f) {
                //white
                hist[1]++;
            } else {
                // check palette colors
                // should be dark and light grey and no black and white found
                // at this point
                float min_sad = FLT_MAX;
                int min_i = 0;
                for (int i = 2; i < n_palette; i++)
                {
                    float sad = fabsf(chroma.x - chroma_palette[i].x)
                        + fabsf(chroma.y - chroma_palette[i].y);
                    if (sad < min_sad)
                    {
                        min_sad = sad;
                        min_i = i;
                    }
                }

                //histogram equalization on Y per primary palette colors

                //debugf("chroma.Y: %f, %d", chroma.Y, min_i);
                //hist[chroma.Y < 0.5f ? min_i : min_i + shade_offset]++;
                hist[min_i]++;
            }
        }
    }

    //debug_array(hist, n);

    int median = median_hist(hist, n_palette, in->n_pixels);
    //int median = median_hist(hist, n, in->n_pixels);
    return median;
}
*/

void
index_image(const image_t *in, image_t *out, color_t *palette, int n_palette)
{
    assert(in != out);
    assert(in->channels == 4);
    assert(out->channels == 1);
    assert(in->width == out->width);
    assert(in->height == out->height);
    assert(n_palette <= 256);

    //float threshold = 30.0f;

    int i_max = in->n_pixels;

    color_t pixel;
    for (int i = 0; i < i_max; i++) {
        get_pixel_at(in, i, &pixel);

        int min_sad = INT_MAX;
        int min_index = 0;
        //double min_diff = DBL_MAX;
        for (int j = 0; j < n_palette; j++) {
            int sad = sad_color(&pixel, &palette[j]);
            if (sad < min_sad) {
            //double diff = diff_color(&pixel, &palette[j]);
            //if (diff < min_diff) {
                min_index = j;
                //min_diff = diff;
                min_sad = sad;
            }
        }

        out->data[i] = min_index;
    }

    /*
    for (int y = 0; y < in->height; y++) {
        for (int x = 0; x < in->width; x++) {
            get_pixel(in, x, y, &pixel);

            float min_diff = DBL_MAX;
            int min_i = 0;
            for (int i = 0; i < n; i++) {
                float diff = diff_ratio_color(&pixel, &palette[i]);
                if (diff < min_diff) {
                    min_diff = diff;
                    min_i = i;
                    if (min_diff < 0.001) {
                        // generally have a match so bail
                        //debugf("min_diff ~0: %f", min_diff);
                        break;
                    }
                }
            }
            //debugf("min_diff: %f", min_diff);
            out->data[y*in->width + x] = min_diff > threshold ? 0 : min_i;
            //max_thr = fmax(min_diff, max_thr);
        }
    }
    */

    //debugf("max_thr: %f", max_thr);
}

void
smooth_index_image(image_t *img)
{
    assert(img->channels == 1);

    int y_max = img->height;
    int x_max = img->width - 1;
    for (int y = 0; y < y_max; y++) {
        int row = y*img->width;
        for (int x = 1; x < x_max; x++) {
            int col = row + x;
            if (img->data[col] != img->data[col - 1] && img->data[col - 1] == img->data[col + 1]) {
                img->data[col] = img->data[col + 1];
                //img->data[col] = 0;
            }
        }
    }
}

// TODO(jason): old and new are bad names
int
mix_image(image_t *old, image_t *new, const int weight)
{
    assert(old->n_pixels == new->n_pixels);
    assert(old->channels == new->channels);

    int max = old->n_pixels * old->channels;

    const int scale = 1000;
    const int new_weight = weight;
    const int old_weight = scale - new_weight;
    for (int i = 0; i < max; i++) {
        old->data[i] = (old_weight * (int)old->data[i] + new_weight * (int)new->data[i])/scale;
    }

    return 0;
}

void
set_all_bgra_image(image_t *img, const u8 r, const u8 g, const u8 b, const u8 a)
{
    assert(img->channels == 4);

    for (int i = 0; i < img->n_pixels*img->channels; i += img->channels) {
        img->data[i] = b;
        img->data[i + 1] = g;
        img->data[i + 2] = r;
        img->data[i + 3] = a;
    }
}

void
set_all_rgb_image(image_t *img, const u8 r, const u8 g, const u8 b)
{
    assert(img->channels == 3);

    for (int i = 0; i < img->n_pixels*img->channels; i += img->channels) {
        img->data[i] = r;
        img->data[i + 1] = g;
        img->data[i + 2] = b;
    }
}

void
set_all_color_image(image_t * img, color_t c)
{
    if (img->channels == 4) {
        set_all_bgra_image(img, c.red, c.green, c.blue, c.alpha);
    }
    else if (img->channels == 3) {
        set_all_rgb_image(img, c.red, c.green, c.blue);
    }
    else if (img->channels == 1) {
        memset(img->data, c.blue, img->n_pixels);
    }
    else {
        debug_s64(img->channels);
        dev_error("unsupported img channels");
    }
}

void
fill_rect(image_t *img, int x, int y, int width, int height, const color_t *fg)
{
    x = clamp(x, 0, img->width - 1);
    y = clamp(y, 0, img->height - 1);

    int max_col = clamp(x + width, 0, img->width);
    int max_row = clamp(y + height, 0, img->height);

    for (int row = y; row < max_row; row++) {
        for (int col = x; col < max_col; col++) {
            set_pixel(img, col, row, fg);
        }
    }
}

void
fill_square_center(image_t *img, int x, int y, int size, const color_t *fg)
{
    fill_rect(img, x - size/2, y - size/2, size, size, fg);
}

void
draw_line_low(image_t *img, int x0, int y0, int x1, int y1, const color_t *fg)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int D = 2*dy - dx;

    int y = y0;
    for (int x = x0; x < x1; x++) {
        set_pixel(img, x, y, fg);
        if (D > 0) {
            y = y + yi;
            D = D - 2*dx;
        }
        D = D + 2*dy;
    }
}

void draw_line_high(image_t *img, int x0, int y0, int x1, int y1, const color_t *fg)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int D = 2*dx - dy;

    int x = x0;
    for (int y = y0; y < y1; y++) {
        set_pixel(img, x, y, fg);
        if (D > 0) {
            x = x + xi;
            D = D - 2*dy;
        }
        D = D + 2*dx;
    }
}

// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
void draw_line(image_t *img, int x0, int y0, int x1, int y1, const color_t *fg)
{
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            draw_line_low(img, x1, y1, x0, y0, fg);
        } else {
            draw_line_low(img, x0, y0, x1, y1, fg);
        }
    } else {
        if (y0 > y1) {
            draw_line_high(img, x1, y1, x0, y0, fg);
        } else {
            draw_line_high(img, x0, y0, x1, y1, fg);
        }
    }
}

void draw_rect(image_t *img, int x, int y, int width, int height, const color_t *fg)
{
    int x2 = clamp(x + width - 1, 0, img->width - 1);
    int y2 = clamp(y + height - 1, 0, img->height - 1);
    x = clamp(x, 0, img->width - 1);
    y = clamp(y, 0, img->height - 1);
    draw_line(img, x, y, x2, y, fg);
    draw_line(img, x, y, x, y2, fg);
    draw_line(img, x2, y, x2, y2, fg);
    draw_line(img, x, y2, x2, y2, fg);
    // TODO(jason): for some reason the bottom right pixel doesn't get drawn.
    set_pixel(img, x2, y2, fg);
}

void
border_image(image_t * img, const color_t * fg)
{
    draw_rect(img, 0, 0, img->width, img->height, fg);
}

void draw_square_center(image_t *img, int x, int y, int size, const color_t *fg)
{
    x = x - size/2;
    y = y - size/2;

    int diff = x + size - img->width;
    int width = (diff > 0) ? size - diff : size;
    diff = y + size - img->height;
    int height = (diff > 0) ? size - diff : size;

    x = clamp(x, 0, img->width - 1);
    y = clamp(y, 0, img->height - 1);

    for (int row = 0; row < height; row++) {
        if (row == 0 || row == height - 1) {
            for (int col = 0; col < width; col++) {
                if (x + col >= img->width) {
                    break;
                }
                set_pixel(img, x + col, y + row, fg);
            }
        } else {
            set_pixel(img, x, y + row, fg);
            set_pixel(img, x + width - 1, y + row, fg);
        }
    }
}

/*
 * draw a line of text.  8x12 character max of 8 points times the size.
 * returns the height of the line plus padding where the next line should be
 * drawn.
 */
/*
 * XXX(jason): problem compiling asteroids_font.h giving "missing braces around
 * initializer" error.  Unclear of fix.  possibly move this function to
 * asteroids_font and somehow fix the problem.
int draw_text(image_t *img, int x, int y, int size, const color_t *fg, const char *text)
{
    const int max_points = 8;
    const int max_height = 12;
    int char_width = 10*size;

    const char *p = text;
    while (*p) {
        // convert lowercase to uppercase
        char c = (*p >= 'a' && *p <= 'z') ? *p & 0xDF : *p;
        const u8 * const pts = asteroids_font[c - ' '].points;

        int x0 = x, y0 = y;
        int x1, y1;

        int next_draw = 0;
        for (int i = 0; i < max_points; i++) {
            u8 delta = pts[i];
            if (delta == FONT_LAST) {
                break;
            }

            if (delta == FONT_UP) {
                // pickup pen, discontinuity
                next_draw = 0;
                continue;
            }

            u8 dx = ((delta >> 4) & 0xF) * size;
            u8 dy = (max_height - ((delta >> 0) & 0xF)) * size;

            x1 = x + dx;
            y1 = y + dy;

            if (next_draw) {
                draw_line(img, x0, y0, x1, y1, fg);
            }

            x0 = x1;
            y0 = y1;
            next_draw = 1;
        }

        p++;
        x += char_width;
    }

    return (max_height + 4) * size;
}
*/

/*
 * draw a line of text with a black shadow (offset x and y by 1)
 */
/*  XXX: see draw_text
int // y offset for next line
draw_shadow_text(image_t *img, int x, int y, int size, const color_t *fg, const char *text)
{
    draw_text(img, x + 1, y + 1, size, &BLACK, text);
    return draw_text(img, x, y, size, fg, text);
}

int draw_debugf(image_t *img, int y, const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 1024, fmt, args);
    va_end(args);

    return draw_shadow_text(img, 2, y, 1, &GREEN, buf);
}

int draw_int(image_t *img, int x, int y, int size, const color_t *fg, int n)
{
    char text[16];
    snprintf(text, 16, "%d", n);
    return draw_text(img, x, y, size, fg, text);
}
*/

// convert a luminance (Y of YUV) to rgba by copying the value into rgb and
// setting a to 0xFF
void
y2rgba(u8 *y, u8 *rgba, int n_pixels)
{
    for (int i = 0, j = 0; i < n_pixels; i++) {
        rgba[j++] = y[i];
        rgba[j++] = y[i];
        rgba[j++] = y[i];
        rgba[j++] = 0xFF;
    }
}

void
gray2rgba_image(image_t *gray, image_t *rgba)
{
    y2rgba(gray->data, rgba->data, gray->n_pixels);
}

/*
 * In this paper they claim "Gleam" (r + g + b)/3 works best for CV
 * https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0029740
 */
void
rgba2gray_image(image_t *rgba, image_t *gray)
{
    assert(rgba->channels == 4);
    assert(gray->channels == 1);

    for (int i = 0; i < rgba->n_pixels; i++) {
        color_t pixel;
        get_pixel_at(rgba, i, &pixel);
        gray->data[i] = (pixel.red + pixel.green + pixel.blue)/3;
    }
}

void
index2rgba_image(const image_t *index, image_t *rgba, color_t *palette, size_t n)
{
    assert(index->channels == 1);
    assert(rgba->channels == 4);
    assert(index->n_pixels == rgba->n_pixels);

    size_t n_pixels = index->n_pixels;

    for (size_t i = 0, j = 0; i < n_pixels; i++) {
        u8 v = index->data[i];
        if (v >= n) {
            log_u64(v, "palette index out of bounds");
            return;
        }
        color_t *c = &palette[v];
        rgba->data[j++] = c->blue;
        rgba->data[j++] = c->green;
        rgba->data[j++] = c->red;
        rgba->data[j++] = c->alpha;
    }
}

void
fy2rgba(float *y, u8 *rgba, int n, float scale)
{
    for (int i = 0, j = 0; i < n; i++) {
        u8 v = (u8)fclampf(y[i]*scale, 0.0f, scale);
        rgba[j++] = v;
        rgba[j++] = v;
        rgba[j++] = v;
        rgba[j++] = 0xFF;
    }
}

void yuyv2rgba(const u8 *yuyv, u8 *rgba, const int n_pixels)
{
    //u32 start_ms = SDL_GetTicks();

    int yuyv_max = n_pixels*2;
    int rgb_max = n_pixels*4;

    for (int i = 0, j = 0; i < yuyv_max && j < rgb_max; i += 4, j += 8) {
        // YUYV: Y0 U0 Y1 V0  Y2 U1 Y3 V1
        u8 Y0 = yuyv[i];
        u8 U = yuyv[i + 1];
        u8 Y1 = yuyv[i + 2];
        u8 V = yuyv[i + 3];

        double vp = 1.402*(V - 128);
        double uvp = -0.344*(U - 128) - 0.714*(V - 128);
        double up = 1.722*(U - 128);

        rgba[j] = clamp255(round(Y0 + up));
        rgba[j + 1] = clamp255(round(Y0 + uvp));
        rgba[j + 2] = clamp255(round(Y0 + vp));
        rgba[j + 3] = 255;

        rgba[j + 4] = clamp255(round(Y1 + up));
        rgba[j + 5] = clamp255(round(Y1 + uvp));
        rgba[j + 6] = clamp255(round(Y1 + vp));
        rgba[j + 7] = 255;
    }

    //debugf("yuyv2rgba: %d", SDL_GetTicks() - start_ms);
}

void yuyv2rgba2(const u8 *yuyv, u8 *rgba, const int n_pixels)
{
    //u32 start_ms = SDL_GetTicks();

    int yuyv_max = n_pixels*2;
    int rgb_max = n_pixels*4;

    for (int i = 0, j = 0; i < yuyv_max && j < rgb_max; i += 4, j += 8) {
        // YUYV: Y0 U0 Y1 V0  Y2 U1 Y3 V1
        int Y0 = yuyv[i];
        int U = yuyv[i + 1] - 128;
        int Y1 = yuyv[i + 2];
        int V = yuyv[i + 3] - 128;

        int vp = V + (V >> 2) + (V >> 3) + (V >> 5);
        int uvp = -((U >> 2) + (U >> 4) + (U >> 5)) - ((V >> 1) + (V >> 3) + (V >> 4) + (V >> 5));
        int up = U + (U >> 1) + (U >> 2) + (U >> 6);

        // bgra
        rgba[j] = clamp255(Y0 + up);
        rgba[j + 1] = clamp255(Y0 + uvp);
        rgba[j + 2] = clamp255(Y0 + vp);
        rgba[j + 3] = 255;

        // bgra
        rgba[j + 4] = clamp255(Y1 + up);
        rgba[j + 5] = clamp255(Y1 + uvp);
        rgba[j + 6] = clamp255(Y1 + vp);
        rgba[j + 7] = 255;
    }

    //debugf("yuyv2rgba2: %d", SDL_GetTicks() - start_ms);
}

void yuyv2gray(const u8 *yuyv, u8 *rgba, const int n_pixels)
{
    //u32 start_ms = SDL_GetTicks();

    int yuyv_max = n_pixels*2;
    int rgb_max = n_pixels*4;

    for (int i = 0, j = 0; i < yuyv_max && j < rgb_max; i += 4, j += 8) {
        // YUYV: Y0 U0 Y1 V0  Y2 U1 Y3 V1
        u8 Y0 = yuyv[i];
        //int U = yuyv[i + 1];
        u8 Y1 = yuyv[i + 2];
        //int V = yuyv[i + 3];

        rgba[j] = Y0;
        rgba[j + 1] = Y0;
        rgba[j + 2] = Y0;
        rgba[j + 3] = 255;

        rgba[j + 4] = Y1;
        rgba[j + 5] = Y1;
        rgba[j + 6] = Y1;
        rgba[j + 7] = 255;
    }

    //debugf("yuyv2gray: %d", SDL_GetTicks() - start_ms);
}

// copy only the luminance values
void
yuyv2y(const u8 *yuyv, u8 *out, const int n_pixels)
{
    const int yuyv_max = n_pixels*2;
    for (int i = 0, j = 0; i < yuyv_max; i += 2, j++) {
        out[j] = yuyv[i];
    }
}

/* Thought the texture would be YUV444, but it's actually 420.
void yuyv2iyuv(const u8 *yuyv, u8 *iyuv, const int width, const int height)
{
    const int n_pixels = width*height;

    //u32 start_ms = SDL_GetTicks();

    // yuyv = yuyvyuyvyuyvyuyv
    // iyuv = yyyyyyyuuvv

    int u_start = n_pixels;
    //int v_start = n_pixels + n_pixels/4;

    for (int i = 0; i < n_pixels; i += 2) {
        // y0uv
        iyuv[i] = yuyv[i]; // y0
        //iyuv[i + u_start] = yuyv[i + 1]; // u
        //iyuv[i + v_start] = yuyv[i + 3]; // v

        // y1uv
        iyuv[i + 1] = yuyv[i + 2]; // y1
        //iyuv[i + 1 + u_start] = yuyv[i + 1]; // u
        //iyuv[i + 1 + v_start] = yuyv[i + 3]; // v
    }

    // black & white
    memset(&iyuv[u_start], 128, n_pixels/2);

    //debugf("yuyv2iyuv: %d ms", SDL_GetTicks() - start_ms);
}
*/

// RGBA to YCbCr
void
rgba2ybr(const u8 *rgba, u8 *ybr, const size_t n_pixels)
{
    size_t i_max = n_pixels * 4;
    for (size_t i = 0, j = 0; i < i_max; i += 4, j += 3) {
        float b = rgba[i];
        float g = rgba[i + 1];
        float r = rgba[i + 2];

        ybr[j] = clamp255(floorf(0.299f * r + 0.587f * g + 0.114f * b));
        ybr[j + 1] = clamp255(floorf(128.f - 0.168736f * r - 0.331264f * g + 0.5f * b));
        ybr[j + 2] = clamp255(floorf(128.f + 0.5f * r - 0.418688f * g - 0.081312f * b));
    }
}

void
set_channel_image(image_t *img, int channel, u8 value)
{
    assert(channel < img->channels);

    size_t i_max = img->n_pixels * img->channels;
    for (size_t i = channel; i < i_max; i += img->channels) {
        img->data[i] = value;
    }
}

// YCbCr to RGBA 
void
ybr2rgba(const u8 *ybr, u8 *rgba, const size_t n_pixels)
{
    size_t i_max = n_pixels * 4;
    for (size_t i = 0, j = 0; i < i_max; i += 4, j += 3) {
        float y = ybr[j];
        float b = ybr[j + 1];
        float r = ybr[j + 2];

        rgba[i + 2] = clamp255(y + 1.402f * (r -128.f));
        rgba[i + 1] = clamp255(y - 0.344136f * (b - 128.f) - 0.714136f * (r - 128));
        rgba[i + 0] = clamp255(y + 1.772f * (b - 128.f));
    }
}

void
checkerboard_yv12(image_t *img, int size)
{
    u8 pixel = 0;
    u8 yfg = 0;
    u8 ybg = 0;

    u8 black = 0x00;
    u8 white = 0xff;

    u8 *data = img->data;
    for (int y = 0; y < img->height; y++) {
        if (y % size == 0) {
            if (yfg == black) {
                yfg = white;
                ybg = black;
            } else {
                yfg = black;
                ybg = white;
            }
        }

        pixel = yfg;
        for (int x = 0; x < img->width; x++) {
            if (x % size == 0) {
                pixel = (pixel == yfg) ? ybg : yfg;
            }

            //debugf("%x %x %x", yfg, ybg, pixel);
            data[y*img->width + x] = pixel;
        }
    }
}

void
checkerboard_image(image_t *img, int size, const color_t *on, const color_t *off)
{
    const color_t *c = on;
    for (int y = 0; y < img->height; y += size) {
        for (int x = 0; x < img->width; x += size) {
            fill_rect(img, x, y, size, size, c);
            c = c == on ? off : on;
        }

        c = c == on ? off : on;
    }
}

// value is -1 to 1
void
draw_hbar(image_t *img, const int x, const int y, const int w, const int h, const color_t *fg, double value)
{
    double percentage = (value + 1.0)/2.0;

    draw_rect(img, x, y, w, h, fg);
    int pw = (int)round(w*percentage);
    //debugf("value: %.3f, percentag: %.3f, pw: %d", value, percentage, pw);
    fill_rect(img, x, y, pw, h, fg);
}

// NOTE(jason): this doesn't appear to be faster than map_window_image that copies
// maybe if it was doing a full image row at a time in windows?
int
map_window_row_image(image_t * out, const image_t * in, u8 map_blob_f(const blob_t * blob), int size)
{
    assert(out != in);
    assert(in->channels == 1);
    assert(in->channels == out->channels);

    // get stride working for min_decimate_image
    //int y_stride = out->height/in->height;
    //int x_stride = out->width/in->width;

    int y_offset = size/2 + 1;
    int x_offset = size/2 + 1;
    int y_max = in->height - size;
    int x_max = in->width - size;

    blob_t row;
    blob_t * inter = stk_blob(size);

    for (int y = 0; y < y_max; y++) {
        for (int x = 0; x < x_max; x++) {
            int stride = out->width;

            for (int y2 = 0; y2 < size; y2++) {
                _init_blob(&row, &in->data[(y + y2)*stride], size, -1, true);
                inter->data[y2] = map_blob_f(&row);
            }

            out->data[(y + y_offset)*stride + x + x_offset] = map_blob_f(inter);
        }
    }

    return 0;
}

// super slow, ignores border
int
map_window_image(const image_t * in, image_t * out, u8 map_f(const image_t * window), image_t * window)
{
    assert(out != in);
    assert(in->channels == 1);
    assert(in->channels == out->channels);
    assert(in->channels == window->channels);

    int y_offset = window->height/2 + 1;
    int x_offset = window->width/2 + 1;
    int y_max = in->height - window->height;
    int x_max = in->width - window->width;

    for (int y = 0; y < y_max; y++) {
        for (int x = 0; x < x_max; x++) {
            roi_image(window, in, x, y);
            out->data[(y + y_offset)*out->width + x + x_offset] = map_f(window);
        }
    }

    return 0;
}

u8
max_image(const image_t * img)
{
    u8 max = 0;
    for (int i = 0; i < img->n_pixels; i++) {
        u8 v = img->data[i];
        if (v > max) max = v;
    }

    return max;
}

u8
min_image(const image_t * img)
{
    u8 min = 255;
    for (int i = 0; i < img->n_pixels; i++) {
        u8 v = img->data[i];
        if (v < min) min = v;
    }

    return min;
}

void
max_window_image(const image_t * in, image_t * out, u8 size_window)
{
    image_t * window = new_image(size_window, size_window, in->channels);

    //map_window_row_image(out, in, max_blob, size_window);
    map_window_image(in, out, max_image, window);

    free_image(window);
}

void
min_window_image(const image_t * in, image_t * out, u8 size_window)
{
    image_t * window = new_image(size_window, size_window, in->channels);

    //map_window_row_image(out, in, min_blob, size_window);
    map_window_image(in, out, min_image, window);

    free_image(window);
}

// pick the most frequent value in the histogram window as the out pixel value
int
max_histogram_decimate_image(const image_t *in, image_t *out)
{
    assert(in->channels == 1);
    assert(in->channels == out->channels);

    int x_stride = in->width/out->width;
    int y_stride = in->height/out->height;
    assert(x_stride > 0);
    assert(y_stride > 0);

    image_t *box = new_image(x_stride, y_stride, 1);

    int hist[256];
    for (int y = 0; y < out->height; y++) {
        for (int x = 0; x < out->width; x++) {
            copy_rect_image(box->width, box->height, in, x*x_stride, y*y_stride, box, 0, 0);
            histogram_image(box, 0, hist, 256);
            //debug_array(hist, 256, "box");
            out->data[y*out->width + x] = index_of_max_array(hist, 256);
        }
    }

    free_image(box);

    return 0;
}

// foreground dilation use value == 255 or whatever gray level.
// for erode use value == 0
int
dilate_image(const image_t *in, image_t *out, const u8 value)
{
    assert(in->channels == 1);
    assert(in->channels == out->channels);

    //const int size = 3;
    const int inset = 1;

    image_t * map = like_image(in);

    for (int y = inset; y < in->height - inset; y++) {
        for (int x = inset; x < in->width - inset; x++) {
            int i = y*in->width + x;

            if (in->data[i] == value) continue;

            int n = (in->data[i - in->width] == value)
                + (in->data[i + in->width] == value)
                + (in->data[i - 1] == value)
                + (in->data[i + 1] == value);
                //+ (in->data[i - in->width - 1] == value)
                //+ (in->data[i - in->width + 1] == value)
                //+ (in->data[i + in->width - 1] == value)
                //+ (in->data[i + in->width + 1] == value);

            if (n > 0) {
                set_pixel(map, x, y, &WHITE);
            }
        }
    }

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int i = y*map->width + x;
            out->data[i] = map->data[i] == 255 ? value : in->data[i];
        }
    }

    free_image(map);

    return 0;
}

// dilate only vertically to keep vertical lines
int
vert_dilate_image(const image_t *in, image_t *out, const u8 value)
{
    assert(in->channels == 1);
    assert(in->channels == out->channels);

    //const int size = 3;
    const int inset = 1;

    image_t * map = like_image(in);

    for (int y = inset; y < in->height - inset; y++) {
        for (int x = inset; x < in->width - inset; x++) {
            int i = y*in->width + x;
            if (in->data[i] != value
                    && (
                        in->data[i - in->width] == value
                        && in->data[i + in->width] == value
                       )
               )
            {
                set_pixel(map, x, y, &WHITE);
            }
        }
    }

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            int i = y*map->width + x;
            out->data[i] = map->data[i] == 255 ? value : in->data[i];
        }
    }

    free_image(map);

    return 0;
}
u8
avg_array_u8(u8 *array, size_t n, size_t stride, size_t offset)
{
    uint sum = 0;
    for (size_t i = offset; i < n; i += stride) {
        sum += array[i];
    }

    return sum/(n/stride);
}

int
avg_decimate_image(image_t *in, image_t *out)
{
    assert(in->channels == out->channels);

    int x_stride = in->width/out->width;
    int y_stride = in->height/out->height;
    assert(x_stride > 0);
    assert(y_stride > 0);

    image_t *box = new_image(x_stride, y_stride, in->channels);

    for (int y = 0; y < out->height; y++) {
        for (int x = 0; x < out->width; x++) {
            copy_rect_image(box->width, box->height, in, x*x_stride, y*y_stride, box, 0, 0);

            // TODO(jason): don't be order dependent, BGRA
            color_t avg;
            avg.blue = avg_array_u8(box->data, box->n_pixels, 4, 0);
            avg.green = avg_array_u8(box->data, box->n_pixels, 4, 1);
            avg.red = avg_array_u8(box->data, box->n_pixels, 4, 2);
            avg.alpha = avg_array_u8(box->data, box->n_pixels, 4, 3);

            set_pixel(out, x, y, &avg);
        }
    }

    // debug
    for (int y = 0; y < out->height; y++) {
        for (int x = 0; x < out->width; x++) {
            color_t c;
            get_pixel(out, x, y, &c);
            fill_rect(in, x*x_stride, y*y_stride, box->width, box->height, &c);
        }
    }

    free_image(box);

    return 0;
}

int
threshold_decimate_image(const image_t *in, image_t *out, u8 max_value, s64 sum_threshold)
{
    assert(in->channels == 1);
    assert(in->channels == out->channels);

    int x_stride = in->width/out->width;
    int y_stride = in->height/out->height;
    assert(x_stride > 0);
    assert(y_stride > 0);

    for (int y = 0; y < out->height; y++) {
        for (int x = 0; x < out->width; x++) {
            s64 sum = 0;
            int xt_off = x*x_stride;
            int yt_off = y*y_stride;
            int xt_max = xt_off + x_stride;
            int yt_max = yt_off + y_stride;

            for (int yt = yt_off; yt < yt_max; yt++) {
                for (int xt = xt_off; xt < xt_max; xt++) {
                    sum += get_gray(in, xt, yt) < max_value;
                }
            }

            //debugf("sum: %zd", sum);
            out->data[y*out->width + x] = (sum > sum_threshold) ? 255 : 0;
        }
    }

    return 0;
}

void
vertical_chart_image(image_t *img, const int *hist, const size_t n, const int x, const int y, const color_t *fg)
{
    int h = 64;

    int vmax = hist[index_of_max_array(hist, 256)];
    if (vmax != 0) {
        for (size_t j = 0; j < n; j++)
        {
            int v = hist[j];
            int y1 = y + h;
            // if there's any value the min is 1
            int l = max(v > 0, (v*h)/vmax);
            int y2 = y1 - l;
            draw_line(img, x + j, y1, x + j, y2, fg);
        }
    }

    draw_rect(img, x - 1, y - 1, n + 2, h + 2, fg);
}

// http://www.tannerhelland.com/4743/simple-algorithm-correcting-lens-distortion/
void
undistort_image(image_t *out, const image_t *in, float strength, float zoom)
{
    struct timespec elapsed = {};
    struct timespec time = {};
    elapsed_debug(&time, NULL, NULL);

    assert(in->width == out->width);
    assert(in->height == in->height);

    float half_width = in->width/2.f;
    float half_height = in->height/2.f;

    if (strength == 0.f) strength = 0.00001f;
    float radius_correction = sqrtf(in->width*in->width + in->height*in->height)/strength;

    float w = in->width;
    float h = in->height;
    for (int y = 0; y < out->height; y++) {
        for (int x = 0; x < out->width; x++) {
            float x_in = x - half_width;
            float y_in = y - half_height;

            float distance = sqrtf(x_in*x_in + y_in*y_in);
            float r = distance/radius_correction;

            float theta;
            if (r > -0.00001 && r < 0.0001f) {
                theta = 1.f;
            } else {
                theta = atanf(r)/r;
            }

            x_in = half_width + theta * x_in * zoom;
            y_in = half_height + theta * y_in * zoom;

            color_t pixel;
            if (x_in >= 0.f && x_in < w && y_in >= 0.f && y_in < h) {
                get_pixel(in, floorf(x_in), floorf(y_in), &pixel);
            } else {
                pixel.red = 0;
                pixel.green = 0;
                pixel.blue = 0;
                pixel.alpha = 0;
            }
            set_pixel(out, x, y, &pixel);
        }
    }

    elapsed_debug(&time, &elapsed, "undistort");
}

// 2d image convolution
// size_kernel is for a single side of a square kernel
void
conv2d_u8_image(u8 *out, u8 *in, size_t width, size_t height, int *kernel, size_t size_kernel) {
    // only odd kernel sizes
    assert(size_kernel % 2 == 1);
    assert(size_kernel >= 3);
    assert(in != NULL);
    assert(out != NULL);
    assert(kernel != NULL);

    size_t offset = size_kernel/2;
    size_t xmax = width - offset;
    size_t ymax = height - offset;
    //debugf("conv2d: %zd %zdx%zd", offset, xmax, ymax);

    int ksum = 0;
    for (size_t i = 0; i < size_kernel*size_kernel; i++) {
        ksum += kernel[i];
    }

    //debugf("ksum: %d", ksum);
    if (ksum == 0) {
        ksum = 1;
    }

    // ignoring size_kernel around edges for now
    for (size_t y = offset; y < ymax; y++) {
        for (size_t x = offset; x < xmax; x++) {
            //debugf("pixel: %zdx%zd", x, y);
            size_t kxmax = x + offset + 1;
            size_t kymax = y + offset + 1;
            // XXX(jason): I don't think this is correct
            // start at end of kernel and decrease to flip kernel horz and vert
            size_t k = size_kernel*size_kernel - 1;
            int sum = 0;
            for (size_t ky = y - offset; ky < kymax; ky++) {
                for (size_t kx = x - offset; kx < kxmax; kx++) {
                    sum += in[ky*width + kx] * kernel[k--];
                }
            }
//            if (sum > 0) {
//            debugf("k max: %zd, sum: %d", k, sum);
//            }
            // threshold?
            out[y*width + x] = clamp255(sum/ksum);
        }
    }
}

void
conv2d_image(image_t * out, image_t * in, int * kernel, size_t size_kernel)
{
    assert(out->width == in->width);
    assert(out->height == in->height);
    assert(out->channels == 1);
    assert(out->channels == in->channels);

    conv2d_u8_image(out->data, in->data, in->width, in->height, kernel, size_kernel);
}

void
box_blur_image(image_t *in, image_t *out, int radius)
{
    assert(radius <= 7);

    int kernel[256];

    size_t size_kernel = radius * 2 + 1;

    for (size_t i = 0; i < size_kernel * size_kernel; i++) {
        kernel[i] = 1;
    }

    conv2d_u8_image(out->data, in->data, in->width, in->height, kernel, size_kernel);
}

void
unsharp_image(image_t *in, image_t *out)
{
    //assert(radius <= 7);

    int kernel[] = {
        1, 4, 6, 4, 1,
        4, 16, 24, 16, 4,
        6, 24, -476, 24, 6,
        4, 16, 24, 16, 4,
        1, 4, 6, 4, 1,
    };
    size_t size_kernel = 5;

    conv2d_u8_image(out->data, in->data, in->width, in->height, kernel, size_kernel);
}


void
edge_detect_image(image_t *in, image_t *out)
{
    int kernel[] = {
        -1, -1, -1,
        -1, 8, -1,
        -1, -1, -1,
    };

    size_t size_kernel = 3;

    conv2d_u8_image(out->data, in->data, in->width, in->height, kernel, size_kernel);
}

void
sharpen_image(image_t *in, image_t *out)
{
    int kernel[] = {
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0,
    };

    size_t size_kernel = 3;

    conv2d_u8_image(out->data, in->data, in->width, in->height, kernel, size_kernel);
}

// max of 255 labels
void
label_connected_components_image(const image_t * img, image_t * labels)
{
    assert(img->channels == 1);
    assert(labels->channels == 1);
    assert(img->width == labels->width);
    assert(img->height == labels->height);

    u8 lut[256] = {};

    const u8 max_label = 255;
    u8 next_label = 1;

    // ignoring the first line which should end up all black
    for (int y = 1; y < labels->height; y++) {
        for (int x = 1; x < labels->width; x++) {
            if (get_gray(img, x, y) != 0) {
                u8 left = get_gray(labels, x - 1, y); // left
                u8 top = get_gray(labels, x, y - 1);

                u8 label = 0;

                if (left == 0 && top == 0) { // new label
                    assert(next_label != max_label);

                    label = next_label;
                    next_label++;

                    lut[label] = label;
                } else if (left != 0 && top != 0) {
                    label = min_u8(min_u8(min_u8(left, top), lut[left]), lut[top]);

                    lut[left] = label;
                    lut[top] = label;
                } else { // one of top or left is foreground
                    label = left != 0 ? left : top;
                }

                set_gray(labels, x, y, label);
            }
        }
    }

    lut_channel_image(labels, labels, lut, 0);
}

ssize_t
write_pam_bgra_image(int fd, const image_t * img)
{
    dev_error(img->channels != 4);

    blob_t * hdr = stk_blob(1024);
    add_blob(hdr, B("P7\n"));
    add_blob(hdr, B("WIDTH "));
    add_s64_blob(hdr, img->width);
    add_blob(hdr, B("\nHEIGHT "));
    add_s64_blob(hdr, img->height);
    add_blob(hdr, B("\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n"));

    size_t n = size_image(img);
    blob_t * rgba = blob(n);
    for (size_t i = 0; i < n; i += img->channels) {
        rgba->data[i] = img->data[i + 2];
        rgba->data[i + 1] = img->data[i + 1];
        rgba->data[i + 2] = img->data[i];
        rgba->data[i + 3] = img->data[i + 3];
    }
    // TODO(jason): i keep directlying updating blobs like I shouldn't and get
    // caught not updating the size.
    rgba->size = n;

    int rc = 0;

    if (write_file_fu(fd, hdr) == -1) {
        rc = -1;
        goto error;
    }

    if (write_full_file_fu(fd, rgba) == -1) {
        log_errno("write_full_file");
        rc = -1;
        goto error;
    }

error:
    free_blob(rgba);

    return rc;
}

ssize_t
write_pam_rgb_image(int fd, const image_t * img)
{
    dev_error(img->channels != 3);

    blob_t * hdr = stk_blob(1024);
    add_blob(hdr, B("P7\n"));
    add_blob(hdr, B("WIDTH "));
    add_s64_blob(hdr, img->width);
    add_blob(hdr, B("\nHEIGHT "));
    add_s64_blob(hdr, img->height);
    add_blob(hdr, B("\nDEPTH 3\nMAXVAL 255\nTUPLTYPE RGB\nENDHDR\n"));

    if (write_file_fu(fd, hdr) == -1) {
        return log_errno("write pam header");
    }

    if (write(fd, img->data, size_image(img)) == -1) {
        return log_errno("write");
    }

    return 0;
}

ssize_t
write_pam_image(int fd, const image_t * img)
{
    if (img->channels == 4) {
        return write_pam_bgra_image(fd, img);
    }
    else if (img->channels == 3) {
        return write_pam_rgb_image(fd, img);
    }
    else {
        debug_s64(img->channels);
        dev_error("unsupported channel count");
    }
}

ssize_t
read_pam_image(int fd, image_t * img)
{
    dev_error(img->channels != 4);

    // XXX(jason): definitely need to fix this
    // 256x256 = 69
    // 1024x256 = 70
    blob_t * hdr = stk_blob(69);
    ssize_t count_hdr = read_file_fu(fd, hdr);
    if (count_hdr == -1) {
        return log_errno("read pam header");
    }
    else if (count_hdr == 0) {
        return 0;
    }

    //debug_blob(hdr);

    if (!begins_with_blob(hdr, B("P7\n"))) {
        error_log("invalid pam file", "pam", 7);
        return -1;
    }

    size_t size = size_image(img);

    ssize_t count = read(fd, img->data, size);
    if (count == -1) {
        log_errno("read pam image");
        return -1;
    }

    if ((size_t)count != size) {
        debug_s64(read);
        debug_s64(size);
        log_errno("read pam image");
        return -1;
    }

    // swap R and B channels
    u8 tmp;
    for (size_t i = 0; i < size; i += 4) {
        tmp = img->data[i];
        img->data[i] = img->data[i + 2];
        img->data[i + 2] = tmp;
    }

    return size;
}

int
save_pam_image(const blob_t * path, const image_t * img)
{
    int rc = 0;

	int fd = open_write_file_fu(path);
	if (fd == -1) {
		rc = log_errno("open_write_file_fu");
        goto error;
	}

	if (write_pam_image(fd, img) == -1) {
        rc = -1;
        goto error;
	}

error:
    close(fd);

	return rc;
}

