#pragma once

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
u8
clamp255(int n)
{
    if (n > 255) return 255;
    if (n < 0) return 0;
    return n;
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

// branchless from https://web.archive.org/web/20190213215419/https://locklessinc.com/articles/sat_arithmetic/
u8
add_sat_u8(u8 a, u8 b)
{
    u8 c = a + b;
    c |= -(c < a);

    return c;
}

// branchless from https://web.archive.org/web/20190213215419/https://locklessinc.com/articles/sat_arithmetic/
u8
sub_sat_u8(u8 a, u8 b)
{
    u8 c = a - b;
    c &= -(c <= a);

    return c;
}

u8
add_sat_s8(u8 a, s8 b)
{
    int c = a + b;
    return clamp255(c);
}

