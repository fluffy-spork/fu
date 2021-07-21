#pragma once

#ifndef SKIP_DEFAULT_INCLUDES_FU
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#ifdef NDEBUG
#define debugf(ignore)((void) 0)
#define debug(ignore)((void) 0)
#else
#define debugf(fmt, ...) \
    do { fprintf(stderr, "D   %s:%s:%d " fmt "\n", __FILE__, __func__, __LINE__, __VA_ARGS__); } while (0)

#define debug(msg) \
    do { fprintf(stderr, "D   %s:%s:%d %s\n", __FILE__, __func__, __LINE__, msg); } while (0)
#endif

/* timespec stuff is included for crude performance timing */

void
debug_timespec(const struct timespec *ts, const char *label)
{
    assert(label != NULL);

    debugf("%s: %ld.%09ld", label, ts->tv_sec, ts->tv_nsec);
}

// return c = a - b
void
sub_timespec(const struct timespec *a, const struct timespec *b, struct timespec *c)
{
    c->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_nsec < b->tv_nsec) {
        c->tv_sec--;
        c->tv_nsec = (a->tv_nsec + 1000000000) - b->tv_nsec;
    } else {
        c->tv_nsec = a->tv_nsec - b->tv_nsec;
    }
}

void
incr_ns_timespec(struct timespec *ts, long ns)
{
    ts->tv_nsec += ns;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_nsec -= 1000000000;
        ts->tv_sec += 1;
    }
}

void
elapsed_debug(struct timespec *last, struct timespec *elapsed, const char *label)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last->tv_sec > 0 && elapsed) {
        sub_timespec(&now, last, elapsed);
        debugf("%s: elapsed %ld.%09ld", label ? label : "no label", elapsed->tv_sec, elapsed->tv_nsec);
    }

    *last = now;
}

/*
bool
gt_timespec(struct timespec *a, struct timespec *b)
{
    return a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec);
}
*/

size_t
copy_cstr(char * dest, size_t size_dest, const char * src, size_t size_src)
{
    size_t size = size_dest < size_src + 1 ? size_dest - 1 : size_src;

    memcpy(dest, src, size);
    dest[size] = '\0';

    return size;
}

