#ifndef FARRAY_FU_H
#define FARRAY_FU_H

struct farray {
    size_t w;
    size_t h;
    size_t size;
    float data[];
};

struct farray *
new_farray(size_t w, size_t h)
{
    size_t size = w*h;
    struct farray *fa = malloc(sizeof(*fa) + size*sizeof(float));
    if (fa) {
        fa->w = w;
        fa->h = h;
        fa->size = size;
    }

    return fa;
}

void
debug_farray(struct farray *in, char *label)
{
    debugf("%s: %zdx%zd = %zd", label, in->w, in->h, in->size);
}

void
copy_rect_farray(size_t width, size_t height, struct farray *out, size_t out_x_off, size_t out_y_off, struct farray *in, size_t in_x_off, size_t in_y_off)
{
    assert(out_x_off + width <= out->w);
    assert(out_y_off + height <= out->h);
    assert(in_x_off + width <= in->w);
    assert(in_y_off + height <= in->h);

    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            out->data[(out_y_off + y)*out->w + out_x_off + x] = in->data[(in_y_off + y)*in->w + in_x_off + x];
        }
    }
}

struct farray *
like_farray(struct farray *a)
{
    return new_farray(a->w, a->h);
}

// possibly should be able to copy a smaller array into a larger as a smaller
// rectangle with propery rows and columns
void
copy_farray(struct farray *out, struct farray *in)
{
    assert(in->size == out->size);

    memcpy(out->data, in->data, in->size*sizeof(float));
}

// convert u8 array to floats and divide by denom
void
copy_u8_farray(struct farray *fa, u8 *in, size_t size, float denom)
{
    assert(fa->size <= size);

    for (size_t i = 0; i < size; i++) {
        fa->data[i] = in[i]/denom;
    }
}

void
reverse_farray(struct farray *out, struct farray *in)
{
    assert(out != in);
    assert(out->size == in->size);
    assert(out->w == in->w);
    assert(out->h == in->h);

    for (size_t i = 0; i < out->size; i++) {
        out->data[i] = in->data[in->size - i - 1];
    }
}

void
hflip_farray(struct farray *out, struct farray *in)
{
    assert(out != in);
    assert(out->size == in->size);
    assert(out->w == in->w);
    assert(out->h == in->h);

    for (size_t y = 0; y < out->h; y++) {
        for (size_t x = 0; x < out->w; x++) {
            out->data[y*out->w + x] = in->data[y*out->w + out->w - x - 1];
        }
    }
}

void
clear_farray(struct farray *a)
{
    memset(a->data, 0, sizeof(float)*a->size);
}

void
set_all_farray(struct farray *a, float f)
{
    for (size_t i = 0; i < a->size; i++) {
        a->data[i] = f;
    }
}

void
scale_1df(float *f, size_t n, float scale)
{
    for (size_t i = 0; i < n; i++) {
        f[i] *= scale;
    }
}

float
faddf(float a, float b)
{
    return a + b;
}

void
div_farray(struct farray *a, struct farray *b, struct farray *c)
{
    for (size_t i = 0; i < a->size; i++) {
        if (b->data[i] != 0.0f) {
            c->data[i] = a->data[i]/b->data[i];
        }
    }
}

void
sub_farray(struct farray *a, struct farray *b, struct farray *c)
{
    for (size_t i = 0; i < a->size; i++) {
        c->data[i] = a->data[i] - b->data[i];
    }
}

void
map_1df(float *out, const float *in, const size_t n, float (*func)(float f))
{
    for (size_t i = 0; i < n; i++) {
        out[i] = (func)(in[i]);
    }
}

void
map_1df_farray(struct farray *a, float (*func)(float f))
{
    map_1df(a->data, a->data, a->size, (func));
}

void
map2_1df(float *out, const float *in1, const float *in2, const size_t n, float (*func)(float f1, float f2))
{
    for (size_t i = 0; i < n; i++) {
        out[i] = (func)(in1[i], in2[i]);
    }
}


float
fsum_2df(float *f, size_t w, size_t h)
{
    const size_t n = w*h;

    float sum = 0.f;
    for (size_t i = 0; i < n; i++) {
        sum += f[i];
    }

    return sum;
}

/*
void
window_2df(float *out, const float *in, size_t width, size_t height, size_t w_win, size_t h_win, float (*func)(float *win, size_t w, size_t h))
{
    float *window = alloca(w_win * h_win);
    size_t x_off = w_win/2;
    size_t y_off = h_win/2;
    for (size_t y = y_off; y < height; y++) {
        for (size_t x = x_off; x < width; x++) {
            // copy into window
            out[y*width + x] = in[y*width + x]
        }
    }

    size_t xoffset = w_win/2;
    size_t yoffset = y_win/2;
    size_t xmax = w - xoffset;
    size_t ymax = h - yoffset;
    //debugf("conv2d: %zd %zdx%zd", offset, xmax, ymax);

    // ignoring kernel over edges for now
    for (size_t y = yoffset; y < ymax; y++) {
        for (size_t x = xoffset; x < xmax; x++) {
            //debugf("pixel: %zdx%zd", x, y);
            size_t kxmax = x + xoffset + 1;
            size_t kymax = y + yoffset + 1;
            // start at end of kernel and decrease to flip kernel horz and vert
            size_t k = kw*kh - 1;
            float sum = 0;
            // think this should be copying src into an array and then dot
            // product of that with the kernel
            for (size_t ky = y - yoffset; ky < kymax; ky++) {
                for (size_t kx = x - xoffset; kx < kxmax; kx++) {
                    sum += in[ky*w + kx] * kernel[k--];
                }
            }
            //debugf("k max: %zd, sum: %d", k, sum);
            // threshold?  clamp?
            out[y*w + x] = sum;
        }
    }
}
*/

/*
void
map_2df(float *out, const float *in, const size_t stride, const size_t w, const size_t h)
{
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
        }
    }
}
*/

void
print_2df(const float *a, const size_t width, const size_t height, const size_t stride)
{
    FILE *stream = stderr;

    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            if (x != 0) {
                fprintf(stream, ", ");
            }
            fprintf(stream, "% .1f", a[y*stride + x]);
        }
        fprintf(stream, "\n");
    }
    fprintf(stream, "\n");
}

void
print_farray(struct farray *fa, size_t max)
{
    print_2df(fa->data, max, max, fa->w);
}

// should probably be called equalize or contrast stretch or range stretch or
// something
void
normalizef(float *f, size_t n, float min_new, float max_new)
{
    float min_old = 0.0f;
    float max_old = 0.0f;

    for (size_t i = 0; i < n; i++) {
        min_old = fminf(min_old, f[i]);
        max_old = fmaxf(max_old, f[i]);
    }

    //debugf("min: %f, max: %f", min_old, max_old);
    float s = (max_new - min_new)/(max_old - min_old);
    // calcuate new values
    for (size_t i = 0; i < n; i++) {
        f[i] = (f[i] - min_old)*s + min_new;
    }
}

// multiply accumulate
float
mac_1df(float *a, float *b, size_t n)
{
    float acc = 0.f;

    for (size_t i = 0; i < n; i++) {
        acc += a[i]*b[i];
    }

    return acc;
}

void
thresholdf(float *f, size_t n, float threshold, float below, float above)
{
    for (size_t i = 0; i < n; i++) {
        f[i] = f[i] < threshold ? below : above;
    }
}

// dividing by the sum "normalizes" so the brightness of the filtered image
// should remain the same as original
void
normalize_kernelf(float *k, size_t n)
{
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += k[i];
    }

    if (sum < -0.001f || sum > 0.001f) {
        for (size_t i = 0; i < n; i++) {
            k[i] /= sum;
        }
    }
}

void
conv2df(float *out, const float *in, const size_t w, const size_t h, const float *kernel, const size_t kw, const size_t kh)
{
    // only odd kernel sizes
    assert(kw % 2 == 1);
    assert(kh % 2 == 1);
    assert(kw > 0);
    assert(kh > 0);
    assert(w > 0);
    assert(h > 0);
    assert(in != NULL);
    assert(out != NULL);
    assert(kernel != NULL);

    size_t xoffset = kw/2;
    size_t yoffset = kh/2;
    size_t xmax = w - xoffset;
    size_t ymax = h - yoffset;
    //debugf("conv2d: %zd %zdx%zd", offset, xmax, ymax);

    // TODO(jason): fix edge handling.  should the out just be smaller than in?
    // this is required since we don't touch the border areas
    memset(out, 0, w*h*sizeof(float));

    // ignoring kernel over edges for now
    for (size_t y = yoffset; y < ymax; y++) {
        for (size_t x = xoffset; x < xmax; x++) {
            //debugf("pixel: %zdx%zd", x, y);
            size_t kxmax = x + xoffset + 1;
            size_t kymax = y + yoffset + 1;
            // start at end of kernel and decrease to flip kernel horz and vert
            size_t k = kw*kh - 1;
            float sum = 0;
            // think this should be copying src into an array and then dot
            // product of that with the kernel
            for (size_t ky = y - yoffset; ky < kymax; ky++) {
                for (size_t kx = x - xoffset; kx < kxmax; kx++) {
                    sum += in[ky*w + kx] * kernel[k--];
                    // TODO(jason): does fmaf help speed?  maybe 1-2 ms.  Need better benchmark
                    //sum = fmaf(in[ky*w + kx], kernel[k--], sum);
                }
            }
            //debugf("k max: %zd, sum: %d", k, sum);
            // threshold?  clamp?
            out[y*w + x] = sum;
        }
    }
}

void
conv_2df_farray(struct farray *out, struct farray *in, const float *kernel, const size_t ksize)
{
    // only odd kernel sizes
    assert(in != NULL);
    assert(out != NULL);
    assert(out->w == in->w);
    assert(out->h == in->h);

    conv2df(out->data, in->data, in->w, in->h, kernel, ksize, ksize);
}

// separable convolution
void
sep_conv_2df(struct farray *out, struct farray *in, const float *vkernel, const float *hkernel, const size_t ksize)
{
    struct farray *tmp = like_farray(out);

    conv2df(tmp->data, in->data, in->w, in->h, vkernel, ksize, 1);
    conv2df(out->data, tmp->data, in->w, in->h, hkernel, 1, ksize);

    free(tmp);
}

void
gaussian_1df(float *f, size_t n, float sigma)
{
    float x0 = n/2;
    float a = 1.f/(sqrtf(2.f*M_PI*sigma*sigma));
    float b = -1.f/(2.f*sigma*sigma);
    for (size_t x = 0; x < n; x++) {
        f[x] = a*expf(b*powf(x - x0, 2.f));
    }
}

void
test_conv2df()
{
    struct farray *in = new_farray(5, 5);
    struct farray *out  = like_farray(in);

    u8 ia[] = {
        105, 102, 100, 97, 96,
        103, 99, 103, 101, 102,
        101, 98, 104, 102, 100,
        99, 101, 106, 104, 99,
        104, 104, 104, 100, 98
    };

    copy_u8_farray(in, ia, 25, 1.0f);

    /*
    for (size_t i = 0; i < in->size; i++) {
        in->data[i] = i;
    }
    */

    //float kernel[9] = { 0, -1, 0, -1, 5, -1, 0, -1, 0 };
    float kernel[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    normalize_kernelf(kernel, 3);
    print_2df(kernel, 3, 3, 3);

    conv_2df_farray(out, in, kernel, 3);
    //sep_conv_2df(out, in, &kernel[3], 3);

    print_farray(in, in->w);
    print_farray(out, in->h);
}

#endif
