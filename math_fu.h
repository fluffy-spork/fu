#ifndef MATH_FU_H
#define MATH_FU_H

u8
add_saturated(u8 a, u8 b)
{
    return (b > 255 - a) ? 255 : a + b;
}

u8
sub_saturated(u8 a, u8 b)
{
    return (a > b) ? a - b : 0;
}

int clamp(int n, int min, int max)
{
    if (n < min) {
        return min;
    } else if (n > max) {
        return max;
    } else {
        return n;
    }
}

double fclamp(double n, double min, double max)
{
    if (n < min) {
        return min;
    } else if (n > max) {
        return max;
    } else {
        return n;
    }
}

float fclampf(float n, float min, float max)
{
    if (n < min) {
        return min;
    } else if (n > max) {
        return max;
    } else {
        return n;
    }
}

// clamp a value to 0 to 255
int clamp255(int n)
{
    return clamp(n, 0, 255);
}

int clamp_int(int a, int min, int max) {
    if (a < min) return min;
    if (a > max) return max;
    return a;
}

double cum_mov_avg(double cma, double x, int n)
{
    return cma + (x - cma)/(n + 1);
}

float cum_mov_avg_f(float cma, float x, int n)
{
    return cma + (x - cma)/(n + 1);
}

#endif
