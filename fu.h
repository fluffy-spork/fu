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
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#if __linux__
    #include <sys/random.h>
#endif

#include <libgen.h>

#include <execinfo.h>
#endif

// NOTE(jason): mainly marking where a response is int with -1 on error so
// distinguishable from int as a value
//
// maybe have to have 2 for -1 error and non-zero error.  although mixing value
// and error in return is not preferred
typedef int error_t;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned long long int u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef long long int s64;

typedef float f32;
typedef double f64;

#define MAX_U64 UINT64_MAX
#define MAX_S64 INT64_MAX

#define for_i(n) for (int i = 0; i < n; i++)
#define for_j(n) for (int j = 0; j < n; j++)

// TODO(jason): I think I want to stop using size_t
#define for_i_size(n) for (size_t i = 0; i < (size_t)n; i++)
#define for_j_size(n) for (size_t j = 0; j < (size_t)n; j++)
#define for_i_offset_size(n, offset) for (size_t i = offset; i < (size_t)n; i++)
#define for_j_offset_size(n, offset) for (size_t j = offset; j < (size_t)n; j++)

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

#define debug_u8(var) debugf("%s: %u/%c", #var, (u8)var, (u8)var);

#define debug_s64(var) debugf("%s: %lld", #var, (s64)var);
#define debug_u64(var) debugf("%s: %llu", #var, (u64)var);

#define debug_item_s64(array, index) debugf("%s[%" PRId64 "]: %" PRId64, #array, (s64)index, (s64)array[index]);
#define debug_hex_item_s64(array, index) debugf("%s[0x%" PRIx64 "]: %" PRId64, #array, (s64)index, (s64)array[index]);

#define debug_hex_u64(var) debugf("%s: 0x%08" PRIx64, #var, (u64)var);
#define debug_hex_u8(var) debugf("%s: 0x%02x", #var, (u8)var);

#define debug_f32(var) debugf("%s: %f", #var, (f32)var);
#define debug_f64(var) debugf("%s: %f", #var, (f64)var);

#define kilobytes(n_bytes) round((double)n_bytes/1024)
#define megabytes(n_bytes) round((double)n_bytes/1024/1024)
#define gigabytes(n_bytes) round((double)n_bytes/1024/1024)

// TODO(jason): seems better than comments, but my initial use seemd off
// putting.  possibly because it looks like code and doesn't have any syntax
// highlighting.  would require custom highlighting
#define NOTE(user, msg)
#define TODO(user, msg)
#define XXX(user, msg)


bool
dev_mode(void)
{
    return getenv("DEV_MODE_FU") != NULL;
}

/* timespec stuff is included for crude performance timing */

int
debug_backtrace_fu(void)
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

struct timespec
zero_timespec(void)
{
    struct timespec zero = {};
    return zero;
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

s64
show_frame_fu(struct timespec * last, s64 frame_duration_ns)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    struct timespec elapsed;
    sub_timespec(&now, last, &elapsed);

    s64 ns = ns_timespec(elapsed);
    if (ns >= frame_duration_ns) {
        *last = now;
        return ns;
    }
    else {
        return 0;
    }
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

int
now_timespec(struct timespec * frame_time)
{
    return clock_gettime(CLOCK_MONOTONIC, frame_time);
}

/* temp remove this as clock_nanosleep isn't on mac
int
frame_delay(struct timespec * frame_time, s32 ns_per_frame)
{
    incr_ns_timespec(frame_time, ns_per_frame);
    return clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, frame_time, NULL);
}
*/

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

int
min_int(int a, int b)
{
    return a < b ? a : b;
}

int
max_int(int a, int b)
{
    return a > b ? a : b;
}

ssize_t
random_fu(void * buf, size_t size_buf)
{
#ifdef __APPLE__
    arc4random_buf(buf, size_buf);
    return size_buf;
#elif __linux__
    return getrandom(buf, size_buf, 0);
#endif
}

u64
random_u64_fu(void)
{
    u64 num;
    ssize_t result = random_fu(&num, sizeof(num));
    RARE_FAIL(result  == sizeof(num));
    return num;
}

s64
random_s64_fu(void)
{
    s64 num;
    ssize_t result = random_fu(&num, sizeof(num));
    num = llabs(num);
    RARE_FAIL(result  == sizeof(num));
    return num;
}

u16
random_u16_fu(void)
{
    u16 num;
    ssize_t result = random_fu(&num, sizeof(num));
    RARE_FAIL(result  == sizeof(num));
    return num;
}

// random alaph character (a-z)
u8
random_alpha_fu(void)
{
    u8 num;
    ssize_t result = random_fu(&num, sizeof(num));
    RARE_FAIL(result  == sizeof(num));
    return 'a' + num % 26;
}

// random between 0 and 1
f64
random_f64_fu(void)
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

