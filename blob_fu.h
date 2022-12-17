#pragma once

// BLOB = binary large object
// variable sized array
//
// thought about a bob_t and blob_t  where bob_t is a fixed size, but then they
// weren't interchangeable which would be annoying.  That's why there are
// small_blob, etc which hopefully work for this purpose.  Also now local_blob,
// etc

// TODO(jason): local_vblob(list of blobs) that allocates a local blob the size
// of the combined list of blobs and adds them all.  basically alloca + concat
// also could do vblob for a heap version


// for isspace.  need to replace
#include <ctype.h>

#define log_blob(value, label) \
    _log(plog, value->data, value->size, label, strlen(label), 0, __FILE__, __func__, __LINE__)

#define log_var_blob(var) log_blob(var, #var)
#define error_log_blob(var) error_log(#var, "error_log_blob", var->error)
// TODO(jason): needs to handle 0 bytes within string and be able to print hex
#define debug_blob(var) debugf("%s: %s", #var, cstr_blob(var))

typedef struct {
    // XXX maybe make this flags?  could track error, pooled, etc
    int error; // 0 = ok, !0 = error code
    size_t capacity; // XXX possibly make size_t ssize_t to not require cast for compare
    size_t size; // XXX possibly make size_t ssize_t to not require cast for compare
    bool constant;
    u8 * data;
} blob_t;

// TODO(jason): maybe this should be called stack_blob?  it's not block local,
// it's stack frame/function
// GCC specific syntax for multiple statements as an expression
#define local_blob(capacity) \
    ({ blob_t * b = alloca(sizeof(blob_t)); u8 * d = alloca(capacity); _init_local_blob(b, d, capacity, 0); })

// NOTE(jason): this version is bad because alloca isn't supposed to be used in
// function calls due to how alloca is typically implemented
//#define local_blob(capacity) _init_local_blob(alloca(sizeof(blob_t)), alloca(capacity), capacity, 0)

#define B(cstr) wrap_blob(cstr)
// GCC specific syntax for multiple statements as an expression
#define wrap_blob(cstr) \
    ({ blob_t * b = alloca(sizeof(blob_t)); _init_local_blob(b, (u8 *)cstr, strlen(cstr), -1); })
#define wrap_array_blob(array) \
    ({ blob_t * b = alloca(sizeof(blob_t)); _init_local_blob(b, (u8 *)array, array_size_fu(array), -1); })

#define cstr_blob(blob) \
    ({ size_t size = blob->size + 1; char * c = alloca(size); _cstr_blob(blob, c, size); })

blob_t *
_init_local_blob(blob_t * b, u8 * data, size_t capacity, ssize_t size)
{
    b->capacity = capacity;
    b->size = size == -1 ? capacity : (size_t)size;
    b->data = data;
    b->error = 0;
    // this isn't true for local_blob
    b->constant = true;

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
    blob->error = 0;
}

void
reset_blob(blob_t * blob)
{
    blob->size = 0;
    blob->error = 0;
}

// TODO(jason): should this just return false if the blob is NULL?  added
// valid_blob to check if a blob has a value
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

char *
_cstr_blob(const blob_t * blob, char * cstr, size_t size_cstr)
{
    assert(blob != NULL);

    ssize_t result = read_blob(blob, cstr, size_cstr);
    // NOTE(jason): this shouldn't ever return an error
    assert(result != -1);

    return cstr;
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
    // TODO(jason): this should probably do something about -1 offset
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

    //assert(offset >= 0);
    if (*poffset < 0) return -1;

    size_t offset = *poffset;

    assert(src->size > 0);
    //debug_u64(offset);
    //debug_u64(src->size);
    //assert(offset <= src->size);
    // NOTE(jason): what the fuck was this assert trying to check???  must've meant
    // remaining_blob as checking to see if there's space  in the src is pointless
    //assert(available_blob(src) >= delim->size);

    if (offset > src->size) return -1;

    // NOTE(jason): if not found returns the whole blob
    size_t size = remaining_blob(src, offset); //src->size - offset;

    size_t imax = src->size;
    for (size_t i = offset; i < imax; i++) {
        // NOTE(jason): there was a check for single char before the memcmp,
        // but i'm assuming that's a premature optimization
        if (memcmp(&src->data[i], delim->data, delim->size) == 0) {
            size = i - offset;
            break;
        }
    }

    if (size) {
        if (write_blob(dest, &src->data[offset], size) == -1) return -1;

        // check for errors!!!
        *poffset = offset + size + delim->size;
        return size;
    } else if (size == 0) {
        // NOTE(jason): delim at first char
        // check for errors!!!
        *poffset = offset + delim->size;
        return 0;
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
scan_line_blob(blob_t * dest, const blob_t * src, ssize_t * poffset)
{
    return scan_blob(dest, src, '\n', poffset);
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

// should take an offset pointer and set it to after the number
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
set_blob(blob_t * b, blob_t * value)
{
    // TODO(jason): should this be clearing the error since reset_blob does?
    // overwrite_blob doesn't
    reset_blob(b);
    return add_blob(b, value);
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

// TODO(jason): is there a point now?  just using B("cstr")  instead does the
// same thing right?  either way using strlen.
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

void
add_s64_blob(blob_t * b, s64 n)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    // possibly should use %jd and cast to intmax_t
    char s[256];
    int size = snprintf(s, 256, "%ld", n);
    write_blob(b, s, size);
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
    int size = snprintf(s, 256, "%lu", n);
    write_blob(b, s, size);
}

void
add_u16_zero_pad_blob(blob_t * b, u64 n)
{
    // TODO(jason): replace with something that doesn't use stdio
    // and direct
    char s[256];
    int size = snprintf(s, 256, "%06lu", n);
    write_blob(b, s, size);
}

bool
ends_with_char_blob(blob_t * b, char c)
{
    return b->data[b->size - 1] == c;
}

bool
begins_with_blob(const blob_t * b, const blob_t * prefix)
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

bool
first_contains_blob(const blob_t * b, const blob_t * target)
{
    ssize_t off = index_blob(b, target->data[0], 0);
    if (off >= 0 && (size_t)remaining_blob(b, off) >= target->size) {
        return memcmp(&b->data[off], target->data, target->size) == 0;
    }

    return false;
}

// XXX: reconsider the ssize_t and -1 to get end of src
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

// XXX: maybe this should be called add_replaced_blob or something???
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
c_escape_blob(blob_t * b, const blob_t * value)
{
    if (!value) return 0;

    char * escape[256] = {};
    escape['\\'] = "\\\\";
    escape['\"'] = "\\\"";
    escape['\b'] = "\\b";
    escape['\f'] = "\\f";
    escape['\n'] = "\\n";
    escape['\r'] = "\\r";
    escape['\t'] = "\\t";

    ssize_t written = 0;
    for (size_t i = 0; i < value->size; i++) {
        u8 c = value->data[i];
        char * v = escape[c];
        if (v) {
            write_blob(b, v, 2);
        }
        else {
            write_blob(b, &c, 1);
        }
    }

    return written;
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

    u8 char_to_hex(u8 c) {
        //assert(c < 16);
        return (c > 9) ? (c - 10) + 'a' : c + '0';
    }

    for (ssize_t i = 0, j = 0; i < n_src; i++, j += 2) {
        u8 c = data[i];
        hex[j] = char_to_hex((c & 0xf0) >> 4);
        hex[j + 1] = char_to_hex(c & 0x0f);
    }

    return write_blob(b, hex, n_hex);
}

#define read_hex_blob_blob(src, dest) read_hex_blob(src, dest->data, dest->size);

ssize_t
read_hex_blob(blob_t * b, void * buf, size_t count)
{
    u8 * hex = b->data;
    u8 * data = buf;

    u8 hex_to_digit(u8 h) {
        //assert((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f'));
        return (h >= '0' && h <= '9') ? h - '0' : 10 + h - 'a';
    }

    for (size_t i = 0, j = 0; i < count; i++, j += 2) {
        u8 upper = hex_to_digit(hex[j]) << 4;
        u8 lower = hex_to_digit(hex[j + 1]);
        data[i] = upper | lower;
    }

    return 2*count;
}

ssize_t
add_random_blob(blob_t * b, ssize_t n_random)
{
    assert(n_random <= 256);

    u8 * r = alloca(n_random);
    if (getrandom(r, n_random, 0) != n_random) {
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

// TODO(jason): would be good if var_name didn't have to be passed around when
// defining enum table

#define ENUM_BLOB(var_name, table) \
    typedef enum { \
        table(var_name, EXTRACT_AS_ENUM_BLOB) \
        n_##var_name \
    } var_name##_t; \
    struct { \
        table(var_name, EXTRACT_AS_VAR_BLOB) \
        size_t n_list; blob_t * list[n_##var_name]; \
    } var_name; \
    void init_##var_name() { \
        table(var_name, EXTRACT_AS_INIT_BLOB) \
        var_name.n_list = n_##var_name; \
    } \
    var_name##_t enum_##var_name(blob_t * value, var_name##_t default_id) { return enum_blob(var_name.list, var_name.n_list, value, default_id); } \
    const blob_t * blob_##var_name(var_name##_t  id) { assert(id < var_name.n_list); return var_name.list[id]; } \

#define EXTRACT_AS_ENUM_BLOB(name, value, var_name) name##_##var_name,
#define EXTRACT_AS_VAR_BLOB(name, value, var_name) blob_t * name;
#define EXTRACT_AS_INIT_BLOB(name, value, var_name) \
    var_name.name = const_blob(value); \
    var_name.list[name##_##var_name] = var_name.name; \

int
enum_blob(blob_t ** enum_blob, size_t n, blob_t * value, int default_id)
{
    // NOTE(jason): enum init method likely needs to be called
    assert(n > 0);

    for (size_t i = 0; i < n; i++) {
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

