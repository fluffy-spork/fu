#pragma once

// BLOB = binary large object
// fixed capacity array
//
// NOTE(jason): not worried about memory usage or immutable.
// For immutable, I use ENUM_BLOBs as constants.
//
// Some things, like constant in blob_t were experiments that could be removed
// or maybe should put error and constant in a u8 flags var or something.
//
// TODO(jason): remove dependencies to C stdlib
// TODO(jason): getrandom required for random_blob, not sure if I care
// TODO(jason): replace size_t with probably s64
// TODO(jason): add some functions for set/get of s16, s32, etc as binary

// for isspace.  need to replace
#include <ctype.h>

#define log_blob(value, label) \
    _log(plog, value->data, value->size, label, strlen(label), 0, __FILE__, __func__, __LINE__)

#define log_var_blob(var) log_blob(var, #var)
#define error_log_blob(var) error_log(#var, "error_log_blob", var->error)

#ifdef NDEBUG
#define debug_blob(ignore)((void)0)
#else
#define debug_blob(var) _debug_blob(#var, (var), __FILE__, __func__, __LINE__)
#endif


#define for_i_blob(b) for_i_size(b->size)
#define for_j_blob(b) for_j_size(b->size)
#define for_i_offset_blob(b, offset) for_i_offset_size(b->size, offset)
#define for_j_offset_blob(b, offset) for_j_offset_size(b->size, offset)

typedef struct {
    // XXX maybe make this flags?  could track error, pooled, etc
    int error; // 0 = ok, !0 = error code
    size_t capacity; // XXX possibly make size_t ssize_t to not require cast for compare
    size_t size; // XXX possibly make size_t ssize_t to not require cast for compare
    bool constant;
    u8 * data;
} blob_t;

// GCC specific syntax for multiple statements as an expression
#define stk_blob(capacity) \
    ({ blob_t * b = alloca(sizeof(blob_t)); u8 * d = alloca(capacity); _init_blob(b, d, capacity, 0, false); })

// stack blob large enough capacity and initialized with an s64
// NOTE(jason): historically, I think this is mostly used like a constant
#define stk_s64_blob(n) \
    ({ blob_t * b = alloca(sizeof(blob_t)); s64 capacity = 32; u8 * d = alloca(capacity); _init_blob(b, d, capacity, 0, false); add_s64_blob(b, n); b; })

#define B(cstr) wrap_blob(cstr)

// TODO(jason): good idea?  for convenience
#define AB(a, b) add_blob(a, B(b))

// TODO(jason): good idea?  removes accidental use of B() with a variable.  why
// should double quote be string identifier.  definitely know it's a string
// constant for array_size_fu
//  EX: SB(foo bar baz) instead of B("foo bar baz")
//  SB for stack blob, so B() should really be malloc/heap?
//  XXX(jason): doesn't actually seem to work correctly in some cases
#define SB(text) \
    ({ blob_t * b = alloca(sizeof(blob_t)); _init_blob(b, (u8 *)#text , array_size_fu(#text) - 1, -1, true); })

// GCC specific syntax for multiple statements as an expression
// wrap_blob should be a function to wrap an existing array with a specified
// size/len and B() should directly do this behavior
#define wrap_blob(cstr) \
    ({ blob_t * b = alloca(sizeof(blob_t)); _init_blob(b, (u8 *)cstr, strlen(cstr), -1, true); })
#define wrap_array_blob(array) \
    ({ blob_t * b = alloca(sizeof(blob_t)); _init_blob(b, (u8 *)array, array_size_fu(array), -1, true); })
#define wrap_size_blob(array, size) \
    ({ blob_t * b = alloca(sizeof(blob_t)); _init_blob(b, (u8 *)array, size, -1, true); })

#define cstr_blob(blob) \
    ({ size_t size = blob->size + 1; char * c = alloca(size); _cstr_blob(blob, c, size); })

blob_t *
_init_blob(blob_t * b, u8 * data, size_t capacity, ssize_t size, bool constant)
{
    b->capacity = capacity;
    b->size = size == -1 ? capacity : (size_t)size;
    b->data = data;
    b->error = 0;
    b->constant = constant;

    return b;
}


blob_t *
blob(size_t capacity)
{
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

void
free_blob(blob_t * blob)
{
    assert(blob->constant == false);

    free(blob);
}


blob_t *
cl_blob()
{
    size_t capacity = 64 - sizeof(blob_t);
    return blob(capacity);
}


// double (64B) cache line blob
// should hold 88 bytes + blob_t in 128B
blob_t *
dcl_blob()
{
    size_t capacity = 2*64 - sizeof(blob_t);
    return blob(capacity);
}


// quad cache line (256B)
blob_t *
qcl_blob()
{
    size_t capacity = 4*64 - sizeof(blob_t);
    return blob(capacity);
}


// page size
blob_t *
page_blob()
{
    size_t capacity = 4096 - sizeof(blob_t);
    debug_u64(capacity);
    return blob(capacity);
}


// NOTE(jason): sets full capacity to 0 to wipe memory.  use reset_blob to just
// abandon data.
void
erase_blob(blob_t * blob)
{
    memset(blob->data, 0, blob->capacity);
    blob->size = 0;
    blob->error = 0;
}

void
reset_blob(blob_t * blob)
{
    blob->size = 0;
    blob->error = 0;
}

int
set_size_blob(blob_t * b, size_t size)
{
    // should this return an error if size > capacity
    b->size = min_size(size, b->capacity);

    return 0;
}

bool
empty_blob(const blob_t * b)
{
    return b->size == 0;
}

bool
valid_blob(const blob_t * b)
{
    return b != NULL && b->size > 0;
}

// TODO(jason): what should this return type be to be a u8 value, but still
// allow returning -1 on error.
// surely there's a better name
// TODO(jason): should be using the blob error field and return u8
s32
offset_blob(const blob_t * b, size_t offset)
{
    if (offset >= b->size) return -1;
    return b->data[offset];
}

// returns -1 if the offset is larger than the current size
//
// TODO(jason): how should this update size.  the previous behavior on return
// b->size++ can't possibly be used anywhere.
ssize_t
set_offset_blob(blob_t * b, size_t offset, u8 value)
{
    if (offset >= b->size) return -1;
    b->data[offset] = value;
    return b->size;
}

// TODO(jason): is this weird and should just convert to an integer?
bool
not_zero_blob(const blob_t * b)
{
    return valid_blob(b) && (b->size != 1 || b->data[0] != '0');
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

// inplace convert the blob to lower case
int
lower_blob(blob_t * b)
{
    for (size_t i = 0; i < b->size; i++) {
        b->data[i] = tolower(b->data[i]);
    }

    return 0;
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
    for (size_t i = b->size - 1; i > 0; i--)
    {
        if (isspace(b->data[i])) {
            new_size--;
        }
        else {
            break;
        }
    }

    if (new_size != b->size) b->size = new_size;
}

void
trim_head_blob(blob_t * b)
{
    size_t n_spaces = 0;
    for (size_t i = 0; i < b->size; i++) {
        if (isspace(b->data[i])) {
            n_spaces++;
        }
        else {
            break;
        }
    }

    if (n_spaces == 0) return;

    if (n_spaces == b->size) {
        reset_blob(b);
        return;
    }

    size_t new_size = b->size - n_spaces;
    memmove(b->data, b->data + n_spaces, new_size);
    b->size = new_size;
}

void
trim_blob(blob_t * b)
{
    trim_tail_blob(b);
    trim_head_blob(b);
}

void
replace_blob(blob_t * b, u8 c, u8 replacement)
{
    for (size_t i = 0; i < b->size; i++) {
        if (b->data[i] == c) b->data[i] = replacement;
    }
}

// also useful for writing single characters: write_blob(b, ":", 1)
//
// TODO(jason): if the blob is too small should this truncate and return an
// error?  currently nothing is written and it returns ENOSPC
// there are potentially a lot of scenarios where truncating would be fine
// and errors could be ignored.
// maybe add a write_trunc_blob?  that's probably a better answer.
// or maybe that would call write_blob that writes the all possible data
// and ignores the ENOSPC error but returns an error on other errors?
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

ssize_t
set_blob(blob_t * b, const blob_t * value)
{
    // TODO(jason): should this be clearing the error since reset_blob does?
    // overwrite_blob doesn't
    reset_blob(b);
    return add_blob(b, value);
}

int
fill_blob(blob_t * b, u8 c, size_t n)
{
    if (n == 0) {
        n = available_blob(b);
    }
    else {
        n = min_size(n, available_blob(b));
    }

    if (n > 0) {
        memset(&b->data[b->size], c, n);

        b->size += n;
    }

    return 0;
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

blob_t *
copy_blob(blob_t * src)
{
    // TODO(jason): is this a good idea?
    if (!src) return NULL;

    // NOTE(jason): errors other than allocation seem unlikely (impossible?)
    blob_t * b = blob(src->size);
    if (b) add_blob(b, src);
    return b;
}

// TODO(jason): implement this to copy the cstr chars until a NUL and not use
// strlen so the advantage would be not iterating the chars twice.
// Only update the size after all chars copied.  handle overrun correctly.
ssize_t
add_cstr_blob(blob_t * b, const char * cstr)
{
    assert(cstr != NULL);

    return write_blob(b, cstr, strlen(cstr));
}


// I keep confusing this for available_blob, but really shouldn't since I don't
// have an offset in those cases.
ssize_t
remaining_blob(const blob_t * src, ssize_t offset)
{
    if (offset < 0) return -1;

    return max_ssize(src->size - offset, 0);
}


ssize_t
read_blob(const blob_t * b, ssize_t offset, void *buf, size_t count)
{
    if (offset < 0) return -1;

    size_t len = min_size(remaining_blob(b, offset), count);

    if (len < 1) return 0;

    memcpy(buf, &b->data[offset], len);

    return len;
}


ssize_t
read_cstr_blob(const blob_t * b, ssize_t offset, void *buf, size_t count)
{
    ssize_t len = read_blob(b, offset, buf, count);
    if (len < 0) return len;

    ssize_t end = min_ssize(len, count - 1);
    ((u8 *)buf)[end] = '\0';

    return len;
}


// similar to read syscall
// copy the bytes into the buffer up to count and ensure null terminated.
// returns an error if buf too small
// maybe add an offset to copy a subset
// maybe should be read_cstr_blob and read_blob as separate functions with
// latter not nul terminating
// XXX(jason): deprecated
ssize_t
____read_blob(const blob_t * b, void *buf, size_t count)
{
    if (b->size + 1 > count) return -1;

    memcpy(buf, b->data, b->size);

    ((u8 *)buf)[b->size] = '\0';

    return b->size;
}


char *
_cstr_blob(const blob_t * blob, char * cstr, size_t size_cstr)
{
    assert(blob != NULL);

    ssize_t result = read_cstr_blob(blob, 0, cstr, size_cstr);
    // NOTE(jason): this shouldn't ever return an error
    assert(result != -1);

    return cstr;
}


void
_debug_blob(const char * var_name, const blob_t * b, const char * file, const char * function, int line)
{
    // TODO(jason): needs to handle 0 bytes within string and be able to print hex

    // XXX: bad, the file and line will be wrong for debugf so need to be passed as parameters
    if (b) {
        fprintf(stderr, "D   %s:%s:%d %s(%lld/%lld): %s\n", file, function, line, var_name, (s64)b->size, (s64)b->capacity, cstr_blob(b));
    }
    else {
        fprintf(stderr, "D   %s:%s:%d %s:NULL\n", file, function, line, var_name);
    }
}


bool
begins_with_char_blob(const blob_t * b, char c)
{
    return b->data[0] == c;
}

bool
ends_with_char_blob(const blob_t * b, char c)
{
    return b->data[b->size - 1] == c;
}

bool
begins_u8_blob(const blob_t * b, u8 v)
{
    return b->data[0] == v;
}

bool
ends_u8_blob(const blob_t * b, u8 v)
{
    return b->data[b->size - 1] == v;
}

bool
ends_with_blob(const blob_t * b, const blob_t * suffix)
{
    if (b->size < suffix->size) return false;

    return memcmp(b->data + (b->size - suffix->size), suffix->data, suffix->size) == 0;
}

bool
begins_with_blob(const blob_t * b, const blob_t * prefix)
{
    if (prefix->size > b->size) return false;

    return memcmp(b->data, prefix->data, prefix->size) == 0;
}

ssize_t
index_blob(const blob_t * b, u8 c, ssize_t offset)
{
    ssize_t imax = b->size;
    for (ssize_t i = offset; i < imax; i++) {
        if (b->data[i] == c) return i;
    }

    return -1;
}

ssize_t
index_blob_blob(const blob_t * b, blob_t * delim, ssize_t offset)
{
    ssize_t imax = b->size;
    for (ssize_t i = offset; i < imax; i++) {
        // NOTE(jason): there was a check for single char before the memcmp,
        // but i'm assuming that's a premature optimization
        if (memcmp(&b->data[i], delim->data, delim->size) == 0) {
            return i;
        }
    }

    return -1;
}

ssize_t
rindex_blob(const blob_t * b, u8 c, ssize_t offset)
{
    if (offset == -1) offset = b->size - 1;

    for (ssize_t i = offset; i >= 0; i--) {
        if (b->data[i] == c) return i;
    }

    return -1;
}

int
trim_ext_blob(blob_t * b)
{
    ssize_t size = rindex_blob(b, '.', -1);
    if (size < 0) {
        return -1;
    }

    b->size = size;

    return 0;
}

ssize_t
replace_ext_blob(blob_t * b, const blob_t * ext)
{
    if (trim_ext_blob(b)) {
        return 0;
    }

    write_blob(b, ".", 1);
    return add_blob(b, ext);
}

// TODO(jason): why is this "first_contains" instead of "contains"?
bool
first_contains_blob(const blob_t * b, const blob_t * target)
{
    ssize_t off = index_blob(b, target->data[0], 0);
    if (off >= 0 && (size_t)remaining_blob(b, off) >= target->size) {
        if (memcmp(&b->data[off], target->data, target->size) == 0) {
            return true;
        }
    }

    return false;
}

// TODO(jason): reconsider the ssize_t and -1 to get end of src
// returns the amount written or -1
ssize_t
sub_blob(blob_t * dest, const blob_t * src, size_t offset, ssize_t size)
{
    assert(offset < src->size);

    if (size == -1) size = remaining_blob(src, offset);

    //assert(dest->size + size <= dest->capacity);
    return write_blob(dest, src->data + offset, size);
}

int
split_blob(const blob_t * src, u8 c, blob_t * b1, blob_t * b2)
{
    ssize_t i = index_blob(src, c, 0);
    if (i == -1) {
        add_blob(b1, src);
    } else {
        sub_blob(b1, src, 0, i);
        i++;
        if ((size_t)i < src->size) sub_blob(b2, src, i, -1);
    }

    return 0;
}

ssize_t
scan_fd_blob(blob_t * b, int fd, u8 delim, size_t max)
{
    // TODO(jason): not sure about this max and buf.  being able to make the
    // max less than the total size seems good, but I'm not sure why it isn't
    // attempting to directly read into the blob?
    assert(max <= 4096);

    size_t size_buf = min_size(available_blob(b), max);
    u8 * buf = alloca(size_buf);
    //u8 buf[4096];
    size_t count = 0;
    u8 c;
    ssize_t n;
    while ((n = read(fd, &c, 1)) == 1 && c != delim)
    {
        buf[count] = c;
        count++;

        if (count == size_buf) break;
    }

    if (count > 0) write_blob(b, buf, count);

    return count;
}

// NOTE(jason): should end of blob count as target?  easier for parsing as
// doesn't have to be a special case
// It can't be as the scanning should indicate that the target was actually
// found.  although maybe with offset pointer the return value identifies
//
// returns the number of bytes written to dest or -1 on error or end of
// blob.  poffset is updated with the index after the target
ssize_t
scan_blob_blob(blob_t * dest, const blob_t * src, blob_t * target, ssize_t * poffset)
{
    assert(dest != NULL);
    assert(src != NULL);
    assert(target != NULL);

    //assert(offset >= 0);
    if (*poffset < 0) return -1;

    size_t offset = *poffset;

    assert(src->size > 0);
    //debug_u64(offset);
    //debug_u64(src->size);
    //assert(offset <= src->size);
    // NOTE(jason): what the fuck was this assert trying to check???  must've meant
    // remaining_blob as checking to see if there's space  in the src is pointless
    //assert(available_blob(src) >= target->size);

    if (offset > src->size) return -1;

    // NOTE(jason): if not found returns the whole blob
    size_t size = remaining_blob(src, offset); //src->size - offset;

    size_t imax = src->size;
    for (size_t i = offset; i < imax; i++) {
        // NOTE(jason): there was a check for single char before the memcmp,
        // but i'm assuming that's a premature optimization
        if (memcmp(&src->data[i], target->data, target->size) == 0) {
            size = i - offset;
            break;
        }
    }

    if (size) {
        if (write_blob(dest, &src->data[offset], size) == -1) return -1;

        // check for errors!!!
        *poffset = offset + size + target->size;
        return size;
    } else if (size == 0) {
        // NOTE(jason): target at first char
        // check for errors!!!
        *poffset = offset + target->size;
        return 0;
    } else {
        return -1;
    }
}

ssize_t
scan_blob(blob_t * dest, const blob_t * src, u8 delim, ssize_t * poffset)
{
    blob_t * tmp = stk_blob(1);
    tmp->data[0] = delim;
    tmp->size = 1;

    return scan_blob_blob(dest, src, tmp, poffset);
}

// delim is multiple single char delimiter chars so that one is matched
// TODO(jason): minimally tested, but likely ok as the only line changed from
// scan_blob_blob is the memcmp -> index_blob.  didn't seem like a good way to
// use the same method and probably better to just be separate. maybe later.
// TODO(jason): maybe a generic function could take multiple ranges and the
// delim case would just have the begin and end of each range the same single
// character.  would take twice as many compares, but I think most cases would
// use a range like whitespace, digits, alpha, etc.  then implemente wrappers
// for delim, whitespace, single range, etc
ssize_t
scan_delim_blob(blob_t * dest, const blob_t * src, blob_t * delim, ssize_t * poffset)
{
    assert(dest != NULL);
    assert(src != NULL);
    assert(delim != NULL);

    if (*poffset < 0) return -1;

    size_t offset = *poffset;

    assert(src->size > 0);

    if (offset > src->size) return -1;

    // NOTE(jason): if not found returns the whole blob
    size_t size = remaining_blob(src, offset); //src->size - offset;

    size_t imax = src->size;
    for (size_t i = offset; i < imax; i++) {
        if (index_blob(delim, src->data[i], 0) != -1) {
            size = i - offset;
            break;
        }
    }

    if (size) {
        if (write_blob(dest, &src->data[offset], size) == -1) return -1;

        // check for errors!!!
        *poffset = offset + size + 1;
        return size;
    } else if (size == 0) {
        // NOTE(jason): target at first char
        // check for errors!!!
        *poffset = offset + 1;
        return 0;
    } else {
        return -1;
    }
}

// scan until whitespace
ssize_t
scan_whitespace_blob(blob_t * dest, const blob_t * src, ssize_t * poffset)
{
    return scan_delim_blob(dest, src, B(" \n\r\t\v"), poffset);
}

ssize_t
scan_line_blob(blob_t * dest, const blob_t * src, ssize_t * poffset)
{
    return scan_blob(dest, src, '\n', poffset);
}

// scan until a duplicate u8 is found.  the poffset should be set to the first
// character of the duplicate
ssize_t
scan_dupe_blob(blob_t * dest, const blob_t * src, ssize_t * poffset)
{
    if (*poffset < 0) return -1;

    size_t offset = *poffset;

    assert(src->size > 0);

    // end of data
    if (offset >= src->size) return -1;

    // NOTE(jason): if not found returns the whole blob
    size_t size = remaining_blob(src, offset);

    u8 last = src->data[offset];
    size_t imax = src->size;
    for (size_t i = offset + 1; i < imax; i++) {
        u8 v = src->data[i];
        if (v == last) {
            size = (i - offset) - 1;
            break;
        }

        last = v;
    }

    if (size > 0) {
        if (write_blob(dest, &src->data[offset], size) == -1) return -1;
    }

    *poffset = offset + size;

    return size;
}

// return count of skipped c with poffset set to non-matching char
ssize_t
skip_u8_blob(const blob_t * b, u8 c, ssize_t * poffset)
{
    ssize_t offset = *poffset;

    // NOTE(jason): if not found returns the remaining blob
    ssize_t size = remaining_blob(b, offset);

    ssize_t imax = b->size;
    for (ssize_t i = offset; i < imax; i++) {
        //debug_s64(i);
        //debug_s64(b->data[i]);
        if (b->data[i] != c) {
            size = i - offset;
            break;
        }
    }

    *poffset = offset + size;

    return size;
}


bool
whitespace_blob(u8 c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
}


ssize_t
skip_whitespace_blob(const blob_t * b, ssize_t * poffset)
{
    for (ssize_t i = *poffset; i < (ssize_t)b->size; i++) {
        if (!whitespace_blob(b->data[i])) {
            *poffset = i;
            return 0;
        }
    }

    return -1;
}

// TODO(jason): should take an offset pointer and set it to after the number so
// it can be used with scan_blob, etc.  Or should there be scan_s64_blob?
// probably
s64
s64_blob(const blob_t * src, ssize_t offset)
{
    skip_whitespace_blob(src, &offset);

    s64 total = 0;
    bool neg = false;
    if (src->data[offset] == '-') {
        neg = true;
        offset++;
    }

    size_t imax = min_size(src->size, 19); // 19
    for (size_t i = offset; i < imax; i++) {
        u8 c = src->data[i];

        if (c >= '0' && c <= '9') {
            total = total * 10 + (c - '0');
        }
        else {
            break;
        }
    }

    return neg
        ? total * -1
        : total;
}

/*
 * this seems problematic for overflow detection and I'm not sure we ever want
 * to be using u64 anyway
u64
u64_blob(const blob_t * src, ssize_t offset)
{
    skip_whitespace_blob(src, &offset);

    u64 total = 0;
    for (size_t i = offset; i < src->size; i++) {
        u8 c = src->data[i];

        if (c >= '0' && c <= '9') {
            total = total * 10 + (c - '0');
        }
        else {
            break;
        }
    }

    return total;
}
*/

/*
XXX: probably should not do things that would require this.  should add longer
values at a time instead of char at a time.
this is also the same as write_blob(b, &c, 1)
this might also be a for a utf-8 int which might be more useful? add_codepoint?
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


// TODO(jason): good idea?
error_t
fmt_int_blob(blob_t * b, const blob_t * format, int value)
{
    size_t available = available_blob(b);

    int written = snprintf((char *)&b->data[b->size], available, cstr_blob(format), value);

    if (written == -1) {
        return -1;
    }

    b->size += written;

    return 0;
}


// TODO(jason): maybe this should have a way for 0 not be added and make an
// empty blob
void
add_s64_blob(blob_t * b, s64 value)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    // possibly should use %jd and cast to intmax_t
    char s[256];
    int size = snprintf(s, 256, "%lld", value);
    write_blob(b, s, size);
}

void
add_s64_zero_pad_blob(blob_t * b, s64 value, ssize_t width)
{
    blob_t * num = stk_blob(64);

    add_s64_blob(num, value);
    ssize_t padding = width - num->size;
    if (padding > 0) {
        fill_blob(b, '0', padding);
    }
    add_blob(b, num);
}

void
set_s64_blob(blob_t * b, s64 n)
{
    reset_blob(b);
    add_s64_blob(b, n);
}

void
add_u64_blob(blob_t * b, u64 n)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    // possibly should use %ju and cast to uintmax_t
    char s[256];
    int size = snprintf(s, 256, "%llu", n);
    write_blob(b, s, size);
}

void
add_u16_zero_pad_blob(blob_t * b, u16 n)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    char s[256];
    // TODO(jason): was 'd' instead of 'u', untested fix
    // probably wasn't ever an issue since it's larger than 16 bit
    int size = snprintf(s, 256, "%06u", n);
    write_blob(b, s, size);
}


ssize_t
add_decimal_blob(blob_t * b, s64 num, int divisor, int precision)
{
    // TODO(jason): needs improvement

    ssize_t start = b->size;

    lldiv_t r = lldiv(num, divisor);
    add_s64_blob(b, r.quot);
    if (precision > 0 && r.rem != 0) {
        write_blob(b, ".", 1);
        add_s64_zero_pad_blob(b, llabs(r.rem), precision);
    }

    return b->size - start;
}

u8
min_blob(const blob_t * b)
{
    u8 min = 255;
    for (size_t i = 0; i < b->size; i++) {
        u8 v = b->data[i];
        if (v < min) min = v;
    }

    return min;
}

u8
max_blob(const blob_t * b)
{
    u8 max = 0;
    for (size_t i = 0; i < b->size; i++) {
        u8 v = b->data[i];
        if (v > max) max = v;
    }

    return max;
}

// XXX: maybe this should be called replace_blob or something???
ssize_t
escape_blob(blob_t * b, const blob_t * value, const u8 c, const blob_t * replacement)
{
    if (!value) return 0;

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


ssize_t
lut_escape_blob(blob_t * out, const blob_t * in, const blob_t * lut[256])
{
    ssize_t written = 0;
    for (size_t i = 0; i < in->size; i++) {
        u8 c = in->data[i];
        const blob_t * v = lut[c];
        if (v) {
            add_blob(out, v);
        }
        else {
            write_blob(out, &c, 1);
        }
    }

    return written;
}


ssize_t
c_escape_blob(blob_t * b, const blob_t * value)
{
    // TODO(jason): redo to use lut_escape_blob
    if (!value) return 0;

    const int size_esacpe = 2;
    char * escape[256] = {
        [0] = "\\0",
        ['\\'] = "\\\\",
        ['\"'] = "\\\"",
        ['\b'] = "\\b",
        ['\f'] = "\\f",
        ['\n'] = "\\n",
        ['\r'] = "\\r",
        ['\t'] = "\\t"
    };

    ssize_t written = 0;
    for (size_t i = 0; i < value->size; i++) {
        u8 c = value->data[i];
        char * v = escape[c];
        if (v) {
            write_blob(b, v, size_esacpe);
        }
        else {
            write_blob(b, &c, 1);
        }
    }

    return written;
}

#define write_hex_blob_blob(b, s) write_hex_blob(b, s->data, s->size)

u8
_char_to_hex_blob(u8 c) {
    //assert(c < 16);
    return (c > 9) ? (c - 10) + 'a' : c + '0';
}

// NOTE(jason): using hex for short values like 64-bit is better than base64
// since it's simpler and the overhead isn't worth it.
ssize_t
write_hex_blob(blob_t * b, void * src, ssize_t n_src)
{
    u8 * data = src;

    ssize_t n_hex = 2*n_src;
    if ((size_t)n_hex > available_blob(b)) {
        b->error = ENOSPC;
        return -1;
    }

    // TODO(jason): be better if this didn't allocate
    char * hex = alloca(n_hex);

    for (ssize_t i = 0, j = 0; i < n_src; i++, j += 2) {
        u8 c = data[i];
        hex[j] = _char_to_hex_blob((c & 0xf0) >> 4);
        hex[j + 1] = _char_to_hex_blob(c & 0x0f);
    }

    return write_blob(b, hex, n_hex);
}

#define read_hex_blob_blob(src, dest) read_hex_blob(src, dest->data, dest->size);

u8
_hex_to_digit_blob(u8 h) {
    //assert((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f'));
    return (h >= '0' && h <= '9') ? h - '0' : 10 + h - 'a';
}

ssize_t
read_hex_blob(blob_t * b, void * buf, size_t count)
{
    u8 * hex = b->data;
    u8 * data = buf;

    for (size_t i = 0, j = 0; i < count; i++, j += 2) {
        u8 upper = _hex_to_digit_blob(hex[j]) << 4;
        u8 lower = _hex_to_digit_blob(hex[j + 1]);
        data[i] = upper | lower;
    }

    return 2*count;
}

#define debug_hex_blob(b) _debug_hex_blob(b, #b)

int
_debug_hex_blob(const blob_t * b, const char * label)
{
    dev_error(b == NULL);

    size_t n = b->size * 2;
    // TODO(jason): a little wonky
    blob_t * hex = stk_blob(n);
    write_hex_blob_blob(hex, b);
    debugf("%s: %s", label, cstr_blob(hex));
    return hex->error;
}

void
add_hex_s64_blob(blob_t * b, s64 value)
{
    // TODO(jason): stop using snprintf
    char s[256];
    int size = snprintf(s, 256, "%llx", value);
    write_blob(b, s, size);
}

ssize_t
add_random_blob(blob_t * b, ssize_t n_random)
{
    assert(n_random <= 256);

    u8 * r = alloca(n_random);
    if (random_fu(r, n_random) != n_random) {
        return -1;
    }

    return write_hex_blob(b, r, n_random);
}

blob_t *
random_blob(ssize_t n_random)
{
    blob_t * b = blob(n_random*2);
    add_random_blob(b, n_random);
    return b;
}

// fills the remaining capacity with random a-z
int
fill_random_alpha_blob(blob_t * b)
{
    for (size_t i = b->size; i < b->capacity; i++) {
        b->data[i] = random_alpha_fu();
    }

    b->size = b->capacity;

    return 0;
}

int
fill_random_blob(blob_t * b)
{
    add_random_blob(b, available_blob(b)/2);
    return 0;
}

int
alpha_numeric_hyphen_blob(const blob_t * b)
{
    for (size_t i = 0; i < b->size; i++) {
        u8 c = b->data[i];
        if (!isalnum(c) && c != '-') return 0;
    }

    return 1;
}


// TODO(jason): enums can have different sizes on different platforms so maybe
// do a struct with id field or something instead.  I had a bug because x86_64
// used 64-bit and arm is only 32-bit
// TODO(jason): would be good if var_name didn't have to be passed around when
// defining enum table

#define ENUM_BLOB(var_name, table) \
    typedef enum { \
        table(var_name, EXTRACT_AS_ENUM_BLOB) \
        n_##var_name \
    } var_name##_t; \
    struct { \
        table(var_name, EXTRACT_AS_VAR_BLOB) \
        int n_list; blob_t * list[n_##var_name]; \
    } var_name; \
    void init_##var_name(void) { \
        table(var_name, EXTRACT_AS_INIT_BLOB) \
        var_name.n_list = n_##var_name; \
    } \
    var_name##_t enum_##var_name(blob_t * value, var_name##_t default_id) { return enum_blob(var_name.list, var_name.n_list, value, default_id); } \
    const blob_t * blob_##var_name(var_name##_t  id) { assert((int)id < var_name.n_list); return var_name.list[id]; } \

#define EXTRACT_AS_ENUM_BLOB(name, value, var_name) name##_##var_name,
#define EXTRACT_AS_VAR_BLOB(name, value, var_name) blob_t * name;
#define EXTRACT_AS_INIT_BLOB(name, value, var_name) \
    var_name.name = const_blob(value); \
    var_name.list[name##_##var_name] = var_name.name; \

int
enum_blob(blob_t ** enum_blob, int n, blob_t * value, int default_id)
{
    // NOTE(jason): enum init method likely needs to be called
    assert(n > 0);

    for (int i = 0; i < n; i++) {
        //log_var_blob(enum_blob[i]);
        if (equal_blob(enum_blob[i], value)) {
            return i;
        }
    }

    return default_id;
}

#define log_enum_blob(var, e, label) \
    log_u64(e, label); \
    log_blob(var.list[e], label);

#define for_each_enum_blob(var) for (size_t i = 0; i < var.n_list; i++)

u64
sad_blob(blob_t * b1, blob_t * b2)
{
    dev_error(b1->size != b2->size);

    u64 sad = 0;
    for_i_blob(b1) {
        sad += abs((int)b1->data[i] - b2->data[i]);
    }

    return sad;
}


int
blob_env_fu(const blob_t * name, blob_t * value, const blob_t * default_value)
{
    char * v = getenv(cstr_blob(name));
    if (v) {
        add_cstr_blob(value, v);
    }
    else if (default_value) {
        add_blob(value, default_value);
    }

    return 0;
}


int
s64_env_fu(const blob_t * name, s64 * value, const s64 default_value)
{
    blob_t * b = stk_blob(32);

    if (blob_env_fu(name, b, NULL)) {
        return -1;
    }

    if (empty_blob(b)) {
        *value = default_value;
    }
    else {
        *value = s64_blob(b, 0);
    }

    return 0;
}


error_t
newline_blob(blob_t * b)
{
    write_blob(b, "\n", 1);
    return b->error;
}


error_t
add_line_blob(blob_t * b, const blob_t * line)
{
    add_blob(b, line);
    newline_blob(b);
    return b->error;
}


error_t
quote_add_blob(blob_t * dest, const blob_t * src)
{
    write_blob(dest, "'", 1);
//    add_blob(dest, src);
    escape_blob(dest, src, '\'', B("'\\''"));
    write_blob(dest, "'", 1);

    return dest->error;
}

