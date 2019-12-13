#ifndef MATH_FU_H
#define MATH_FU_H

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

double cum_mov_avg(double cma, double x, size_t n)
{
    return cma + (x - cma)/(n + 1);
}

#endif
