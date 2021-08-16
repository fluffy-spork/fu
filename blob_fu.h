#pragma once

// BLOB = binary large object
// variable sized array

// for isspace
#include <ctype.h>

#define log_blob(log, value, label) \
    _log(log, value->data, value->size, label, 0, __FILE__, __func__, __LINE__);

typedef struct {
    // XXX maybe make this flags?  could track error, pooled, etc
    int error; // 0 = ok, !0 = error code
    size_t capacity;
    size_t size;
    u8 data[];
} blob_t;

blob_t *
new_blob(size_t capacity)
{
    assert(capacity > 0);

    size_t n = sizeof(blob_t) + sizeof(u8) * capacity;
    blob_t * blob = malloc(n);
    if (!blob) return NULL;

    blob->capacity = capacity;
    blob->size = 0;
    blob->error = 0;

    return blob;
}

// TODO(jason): by having standard sizes there could be pools
#define SMALL_SIZE_BLOB 256
#define MEDIUM_SIZE_BLOB 4096
#define LARGE_SIZE_BLOB 65536

blob_t *
small_blob()
{
    return new_blob(SMALL_SIZE_BLOB);
}

blob_t *
medium_blob()
{
    return new_blob(MEDIUM_SIZE_BLOB);
}

blob_t *
large_blob()
{
    return new_blob(LARGE_SIZE_BLOB);
}

void
free_blob(blob_t * blob)
{
    free(blob);
}

void
erase_blob(blob_t * blob)
{
    memset(blob->data, 0, blob->size);
    blob->size = 0;
}

void
reset_blob(blob_t * blob)
{
    blob->size = 0;
}

bool
empty_blob(blob_t * b)
{
    assert(b != NULL);

    return b->size == 0;
}

bool
equal_blob(blob_t * b1, blob_t * b2)
{
    return b1->size == b2->size && memcmp(b1->data, b2->data, b1->size) == 0;
}

size_t
available_blob(blob_t * b)
{
    return b->capacity - b->size;
}

// remove trailing whitespace.  the size is just decreased
void
trim_tail_blob(blob_t * b)
{
    size_t new_size = b->size;
    for (size_t i = b->size -1; i > 0; i--)
    {
        if (isspace(b->data[i])) new_size--;
    }

    if (new_size != b->size) b->size = new_size;
}

// also useful for writing single characters: write_blob(b, ":", 1)
ssize_t
write_blob(blob_t * b, const void * buf, const size_t count)
{
    assert(b != NULL);
    assert(buf != NULL);

    if (b->error) return -1;

    size_t new_size = b->size + count;

    if (new_size > b->capacity)
    {
        b->error = ENOSPC;
        return -1;
    }

    memcpy(&b->data[b->size], buf, count);
    b->size = new_size;

    return count;
}

// similar to read syscall
// copy the bytes into the buffer up to count and ensure null terminated.
// returns an error if buf too small
// maybe add an offset to copy a subset
// maybe should be read_cstr_blob and read_blob as separate functions with
// latter not nul terminating
ssize_t
read_blob(const blob_t * b, void *buf, size_t count)
{
    if (b->size + 1 > count) return -1;

    memcpy(buf, b->data, b->size);

    ((u8 *)buf)[b->size] = '\0';

    return b->size;
}

// write into the blob as much of the fd that fits
ssize_t
write_fd_blob(blob_t * b, int fd)
{
    assert(b != NULL);

    u8 * buf = &b->data[b->size];
    size_t available = available_blob(b);

    // TODO(jason): it's possible there could be more to read that would fit
    // than is returned in one read call.  maybe use a do/while
    ssize_t size = read(fd, buf, available);
    if (size > 0) b->size += size;

    // size 0 means an EOF was read.
    return size;
}

ssize_t
overwrite_blob(blob_t * dest, const blob_t * src)
{
    assert(dest != src);

    dest->size = 0;
    return write_blob(dest, src->data, src->size);
}

blob_t *
const_blob(const char *cstr)
{
    if (!cstr) return NULL;

    size_t size = strlen(cstr);

    blob_t * b = new_blob(size);

    write_blob(b, cstr, size);

    return b;
}

ssize_t
add_blob(blob_t * b, blob_t * b2)
{
    return write_blob(b, b2->data, b2->size);
}

// NOTE(jason): wrapper so it's easy to not have to know the trailing NULL is
// required for var args in C.  Also means a NULL can't be included, but that
// shouldn't really matter.
#define vadd_blob(t, ...) _vadd_blob(t, __VA_ARGS__, NULL)

// NOTE(jason): va_args must end with NULL
// TODO(jason): this should possibly return ssize_t that's a total of all
// added.  however, I'm not sure it matters since the before and after size is
// available if somebody wants
ssize_t
_vadd_blob(blob_t * b, ...)
{
    va_list args;
    va_start(args, b);

    blob_t * b2;
    while ((b2 = va_arg(args, blob_t *)))
    {
        // NOTE(jason): bail on error
        if (add_blob(b, b2) == -1) return -1;
    }

    va_end(args);

    return 0;
}

ssize_t
add_cstr_blob(blob_t * b, const char * cstr)
{
    assert(cstr != NULL);

    return write_blob(b, cstr, strlen(cstr));
}

void
add_s64_blob(blob_t * b, s64 n)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    char s[256];
    int size = snprintf(s, 256, "%ld", n);
    write_blob(b, s, size);
}

void
add_u64_blob(blob_t * b, u64 n)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    char s[256];
    int size = snprintf(s, 256, "%lu", n);
    write_blob(b, s, size);
}

bool
ends_with_char_blob(blob_t * b, char c)
{
    return b->data[b->size - 1] == c;
}

bool
begins_with_blob(blob_t * b, blob_t * prefix)
{
    if (prefix->size > b->size) return false;

    return memcmp(b->data, prefix->data, prefix->size) == 0;
}

ssize_t
index_blob(blob_t * b, u8 c)
{
    for (size_t i = 0; i < b->size; i++) {
        if (b->data[i] == c) return i;
    }

    return -1;
}

// XXX: reconsider the ssize_t and -1 to get end of src
int
sub_blob(blob_t * dest, const blob_t * src, size_t offset, ssize_t size)
{
    assert(offset < src->size);

    if (size == -1) size = src->size - offset;

    //assert(dest->size >= size);

    memcpy(dest->data, src->data + offset, size);
    dest->size = size;

    return 0;
}

int
split_blob(blob_t * src, u8 c, blob_t * b1, blob_t * b2)
{
    ssize_t i = index_blob(src, c);
    sub_blob(b1, src, 0, i);
    sub_blob(b2, src, i + 1, -1);

    return 0;
}

