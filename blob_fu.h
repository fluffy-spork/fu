#pragma once

// BLOB = binary large object
// variable sized array
//
// thought about a bob_t and blob_t  where bob_t is a fixed size, but then they
// weren't interchangeable which would be annoying.  That's why there are
// small_blob, etc which hopefully work for this purpose.  Also now local_blob,
// etc

// for isspace.  need to replace
#include <ctype.h>

#define log_blob(value, label) \
    _log(plog, value->data, value->size, label, strlen(label), 0, __FILE__, __func__, __LINE__);

#define log_var_blob(var) log_blob(var, #var);


typedef struct {
    // XXX maybe make this flags?  could track error, pooled, etc
    int error; // 0 = ok, !0 = error code
    size_t capacity; // XXX possibly make size_t ssize_t to not require cast for compare
    size_t size; // XXX possibly make size_t ssize_t to not require cast for compare
    bool constant;
    u8 * data;
} blob_t;

#define local_blob(size) _init_local_blob(&(blob_t){ .data = alloca(size) }, size, 0)
#define tmp_blob() local_blob(4096)

#define wrap_blob(cstr) _init_local_blob(&(blob_t){ .data = (u8 *)cstr }, strlen(cstr), -1)

blob_t *
_init_local_blob(blob_t * b, size_t capacity, ssize_t size)
{
    b->capacity = capacity;
    b->size = size == -1 ? capacity : (size_t)size;
    b->constant = true;

    return b;
}

blob_t *
blob(size_t capacity)
{
    assert(capacity > 0);

    blob_t * b;

    size_t n = sizeof(*b) + sizeof(*b->data) * capacity;
    b = malloc(n);
    if (!b) return NULL;

    // TODO(jason): is this correct?
    b->data = (u8 *)b + sizeof(*b);
    b->capacity = capacity;
    b->constant = false;
    b->size = 0;
    b->error = 0;

    return b;
}

// TODO(jason): by having standard sizes there could be pools
// maybe should be sizes in the names instead of "small", etc
// these probably aren't necessary anymore after adding local_blob
#define SMALL_SIZE_BLOB 256
#define MEDIUM_SIZE_BLOB 4096
#define LARGE_SIZE_BLOB 65536

blob_t *
small_blob()
{
    return blob(SMALL_SIZE_BLOB);
}

blob_t *
medium_blob()
{
    return blob(MEDIUM_SIZE_BLOB);
}

blob_t *
large_blob()
{
    return blob(LARGE_SIZE_BLOB);
}

void
free_blob(blob_t * blob)
{
    assert(blob->constant == false);

    free(blob);
}

// maybe clear? set everything to a specific char?
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
empty_blob(const blob_t * b)
{
    assert(b != NULL);

    return b->size == 0;
}

bool
equal_blob(const blob_t * b1, const blob_t * b2)
{
    return b1->size == b2->size && memcmp(b1->data, b2->data, b1->size) == 0;
}

// FIX: gotta be a faster way.  right?
bool
case_equal_blob(const blob_t * b1, const blob_t * b2)
{
    if (b1->size != b2->size) return false;

    for (size_t i = 0; i < b1->size; i++) {
        if (tolower(b1->data[i]) != tolower(b2->data[i])) {
            return false;
        }
    }

    return true;
}

size_t
available_blob(const blob_t * b)
{
    return b->capacity - b->size;
}

// remove trailing whitespace.  the size is just decreased
// should this be right_trim and left_trim?
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

void
replace_blob(blob_t * b, u8 c, u8 replacement)
{
    for (size_t i = 0; i < b->size; i++) {
        if (b->data[i] == c) b->data[i] = replacement;
    }
}

// also useful for writing single characters: write_blob(b, ":", 1)
ssize_t
write_blob(blob_t * b, const void * buf, const size_t count)
{
    if (count == 0) return 0;

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

ssize_t
overwrite_blob(blob_t * b, const void * buf, const size_t count)
{
    b->size = 0;
    return write_blob(b, buf, count);
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
// TODO(jason): this might should be called "read_fd_blob".  was trying to
// match behavior to write_blob, but maybe that's a bad idea.  A "read_fd_blob"
// that reads from the blob and writes into an fd seems more confusing
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
scan_fd_blob(blob_t * b, int fd, u8 delim, size_t max)
{
    assert(max <= MEDIUM_SIZE_BLOB);

    u8 buf[MEDIUM_SIZE_BLOB];
    size_t count = 0;
    u8 c;
    ssize_t n;
    while ((n = read(fd, &c, 1)) == 1 && c != delim)
    {
        buf[count] = c;
        count++;
    }

    if (count > 0) write_blob(b, buf, count);

    return count;
}

ssize_t
remaining_blob(const blob_t * src, ssize_t offset)
{
    return src->size - offset;
}

// NOTE(jason): should end of blob count as delimiter?  easier for parsing as
// doesn't have to be a special case
// It can't be as the scanning should indicate that the delim was actually
// found.  although maybe with offset pointer the return value identifies
//
// returns the number of bytes written to dest or -1 on error or end of
// blob.  poffset is updated with the index after the delim
ssize_t
scan_blob_blob(blob_t * dest, const blob_t * src, blob_t * delim, ssize_t * poffset)
{
    assert(dest != NULL);
    assert(src != NULL);
    assert(delim != NULL);

    ssize_t offset = *poffset;

    // XXX: for now.  this method should return -1 on error eventually
    // what can cause an error?
    // possibly return error on not enough space in dest?
    assert(offset >= 0);
    assert(src->size > 0);
    //assert(offset <= (ssize_t)src->size);
    assert(available_blob(src) >= delim->size);

    if (offset > (ssize_t)src->size) return -1;

    // NOTE(jason): if not found returns the whole blob
    ssize_t size = src->size - offset;

    ssize_t imax = (ssize_t)(src->size);
    for (ssize_t i = offset; i < imax; i++) {
        if (src->data[i] == delim->data[0]
                && (delim->size == 1 || memcmp(&src->data[i], delim->data, delim->size) == 0))
        {
            size = i - offset;
            break;
        }
    }

    if (size) {
        write_blob(dest, &src->data[offset], size);
        *poffset = offset + size + delim->size;
        return size;
    } else {
        return -1;
    }
}

ssize_t
scan_blob(blob_t * dest, const blob_t * src, u8 delim, ssize_t * poffset)
{
    blob_t * tmp = local_blob(1);
    tmp->data[0] = delim;
    tmp->size = 1;

    return scan_blob_blob(dest, src, tmp, poffset);
}

ssize_t
skip_whitespace_blob(const blob_t * b, ssize_t * poffset)
{
    for (ssize_t i = *poffset; i < (ssize_t)b->size; i++) {
        if (!isspace(b->data[i])) {
            *poffset = i;
            return 0;
        }
    }

    return -1;
}

u64
u64_blob(const blob_t * src, ssize_t offset)
{
    skip_whitespace_blob(src, &offset);

    u64 total = 0;

    for (ssize_t i = offset; i < (ssize_t)src->size; i++) {
        u8 c = src->data[i];

        if (c >= '0' && c <= '9') {
            total = total * 10 + (c - '0');
        } else {
            break;
        }
    }

    return total;
}


// TODO(jason): maybe this should be different like cstr_blob() or something
// should this return a "const blob_t *" or similar?
blob_t *
const_blob(const char *cstr)
{
    if (!cstr) return NULL;

    size_t size = strlen(cstr);

    blob_t * b = blob(size);
    // XXX(jason): should this be set?
    b->constant = true;

    write_blob(b, cstr, size);

    return b;
}

ssize_t
add_blob(blob_t * dest, const blob_t * src)
{
    return write_blob(dest, src->data, src->size);
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

/*
XXX: probably should not do things that would require this.  should add longer
values at a time instead of char at a time.
this is also the same as write_blob(b, &c, 1)
void
add_u8_blob(blob_t * b, u8 c)
{
    size_t new_size = b->size + 1;

    if (new_size > b->capacity)
    {
        b->error = ENOSPC;
        return -1;
    }

    b->data[b->size] = c;
    b->size = new_size;
}
*/

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
index_blob(const blob_t * b, u8 c, size_t offset)
{
    for (size_t i = offset; i < b->size; i++) {
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
split_blob(const blob_t * src, u8 c, blob_t * b1, blob_t * b2)
{
    ssize_t i = index_blob(src, c, 0);
    if (i == -1) {
        add_blob(b1, src);
    } else {
        sub_blob(b1, src, 0, i);
        sub_blob(b2, src, i + 1, -1);
    }

    return 0;
}

// XXX: maybe this should be called add_replaced_blob or something???
ssize_t
escape_blob(blob_t * b, const blob_t * value, const u8 c, const blob_t * replacement)
{
    ssize_t written = 0;

    ssize_t offset = 0;
    ssize_t i = 0;
    while ((i = index_blob(value, c, offset)) != -1)
    {
        written += write_blob(b, &value->data[offset], i - offset);
        written += add_blob(b, replacement);
        offset = i + 1;
    }

    written += write_blob(b, &value->data[offset], value->size - offset);

    return written;
}

