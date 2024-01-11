#pragma once

#ifndef SKIP_DEFAULT_INCLUDES_FU
// TODO(jason): replace assert with something better, maybe dev_error
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>
#include <unistd.h>

#include <libgen.h>

#include <execinfo.h>
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

#define MAX_U64 UINT64_MAX
#define MAX_S64 INT64_MAX

#define for_i(n) for (int i = 0; i < n; i++)
#define for_j(n) for (int j = 0; j < n; j++)

// TODO(jason): I think I want to stop using size_t
#define for_i_size(n) for (size_t i = 0; i < (size_t)n; i++)
#define for_j_size(n) for (size_t j = 0; j < (size_t)n; j++)

typedef enum {
    U8_DATA_TYPE,
    U16_DATA_TYPE,
    U32_DATA_TYPE,
    U64_DATA_TYPE,
    S8_DATA_TYPE,
    S16_DATA_TYPE,
    S32_DATA_TYPE,
    S64_DATA_TYPE,
    F32_DATA_TYPE,
    F64_DATA_TYPE,
    CSTR_256_DATA_TYPE,
    CSTR_1K_DATA_TYPE,
    CSTR_4K_DATA_TYPE,
    CSTR_64K_DATA_TYPE,
    BLOB_DATA_TYPE
} data_type_t;

// mark a variable as unused to since warnings are set to errors
#define UNUSED(var) (void)var

#define assert_not_null(var) assert(var != NULL);

// crash on a rare error situation that shouldn't ever happen and if does
// everything is probably fucked
// needs to not use assert so exp is always run.
#define RARE_FAIL(exp) assert(exp)

// NOTE(jason): this developer has an error that must be fixed but could only
// be checked at runtime
// inverts the check used for assert so that a msg can be the exp or an actual
// expression
// dev_error("missing param foo")
// dev_error(user_id < 1) vs assert(user_id > 0)
#define dev_error(exp) assert(!(exp))

#define array_size_fu(array) sizeof(array)/sizeof(array[0])

#ifdef NDEBUG
#define debugf(ignore, ...)((void) 0)
#define debug(ignore)((void) 0)
#else
#define debugf(fmt, ...) \
    do { fprintf(stderr, "D   %s:%s:%d " fmt "\n", __FILE__, __func__, __LINE__, __VA_ARGS__); } while (0)

#define debug(msg) \
    do { fprintf(stderr, "D   %s:%s:%d %s\n", __FILE__, __func__, __LINE__, msg); } while (0)
#endif

#define debug_s64(var) debugf("%s: %ld", #var, (s64)var);
#define debug_u64(var) debugf("%s: %lu", #var, (u64)var);

#define debug_hex_u64(var) debugf("%s: %lx", #var, (u64)var);

#define debug_f64(var) debugf("%s: %f", #var, (f64)var);

#define kilobytes(n_bytes) round((double)n_bytes/1024)
#define megabytes(n_bytes) round((double)n_bytes/1024/1024)
#define gigabytes(n_bytes) round((double)n_bytes/1024/1024)

bool
dev_mode()
{
    return getenv("FU_DEV") != NULL;
}

/* timespec stuff is included for crude performance timing */

int
debug_backtrace_fu()
{
    const int BT_BUF_SIZE = 1024;
    void *buffer[BT_BUF_SIZE];
    int nptrs;

    nptrs = backtrace(buffer, BT_BUF_SIZE);
    debugf("\n\nbacktrace() returned %d addresses", nptrs);

    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);

    return 0;
}

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

s64
ns_timespec(struct timespec ts)
{
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

s64
elapsed_ns(struct timespec start)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    struct timespec elapsed;
    sub_timespec(&now, &start, &elapsed);

    return ns_timespec(elapsed);
}

void
elapsed_debug(struct timespec *last, struct timespec *elapsed, const char *label)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    if (last->tv_sec > 0 && elapsed) {
        sub_timespec(&now, last, elapsed);
        debugf("%s: elapsed %ld.%09ld", label ? label : "no label", elapsed->tv_sec, elapsed->tv_nsec);
    }

    *last = now;
}

f64
f64_sec_timespec(const struct timespec ts)
{
    const f64 nsec_per_sec = 1000 * 1000 * 1000;
    return (f64)ts.tv_sec + ((f64)ts.tv_nsec)/nsec_per_sec;
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

size_t
min_size(size_t a, size_t b)
{
    return a < b ? a : b;
}

size_t
max_size(size_t a, size_t b)
{
    return a > b ? a : b;
}

ssize_t
min_ssize(ssize_t a, ssize_t b)
{
    return a < b ? a : b;
}

ssize_t
max_ssize(ssize_t a, ssize_t b)
{
    return a > b ? a : b;
}

u8
min_u8(u8 a, u8 b)
{
    return a < b ? a : b;
}

u8
max_u8(u8 a, u8 b)
{
    return a > b ? a : b;
}

s8
min_s8(s8 a, s8 b)
{
    return a < b ? a : b;
}

s8
max_s8(s8 a, s8 b)
{
    return a > b ? a : b;
}

s16
min_s16(s16 a, s16 b)
{
    return a < b ? a : b;
}

s16
max_s16(s16 a, s16 b)
{
    return a > b ? a : b;
}

s32
min_s32(s32 a, s32 b)
{
    return a < b ? a : b;
}

s32
max_s32(s32 a, s32 b)
{
    return a > b ? a : b;
}

s64
min_s64(s64 a, s64 b)
{
    return a < b ? a : b;
}

s64
max_s64(s64 a, s64 b)
{
    return a > b ? a : b;
}

u64
random_u64_fu()
{
    u64 num;
    ssize_t result = getrandom(&num, sizeof(num), 0);
    RARE_FAIL(result  == sizeof(num));
    return num;
}

s64
random_s64_fu()
{
    s64 num;
    ssize_t result = getrandom(&num, sizeof(num), 0);
    num = llabs(num);
    RARE_FAIL(result  == sizeof(num));
    return num;
}

u16
random_u16_fu()
{
    u16 num;
    ssize_t result = getrandom(&num, sizeof(num), 0);
    RARE_FAIL(result  == sizeof(num));
    return num;
}

// random alaph character (a-z)
u8
random_alpha_fu()
{
    u8 num;
    ssize_t result = getrandom(&num, sizeof(num), 0);
    RARE_FAIL(result  == sizeof(num));
    return 'a' + num % 26;
}

// random between 0 and 1
f64
random_f64_fu()
{
    u64 r = random_u64_fu();

    return (f64)r/(f64)MAX_U64;
}

// https://c-faq.com/lib/randrange.html
// M + rand() / (RAND_MAX / (N - M + 1) + 1);
s64
random_range_s64_fu(s64 min, s64 max)
{
    return min + random_u64_fu() / (MAX_U64 / (max - min + 1) + 1);
}

s16
abs_s16(s16 a)
{
    return abs(a);
}

// TODO(jason): should this return u64?
s64
abs_s64(s64 a)
{
    return llabs(a);
}

