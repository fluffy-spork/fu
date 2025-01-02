#pragma once

// sudo apt install libdeflate-dev
// add $(pkgconf --cflags --libs libdeflate) to jcc.env lib_flags
//
// TODO(jason): replace with an internal implementation

#include <libdeflate.h>


int
gzip_fu(blob_t * dest, const blob_t * src)
{
    struct libdeflate_compressor * comp = libdeflate_alloc_compressor(12);
    if (!comp) return -1;

    size_t size = libdeflate_gzip_compress(comp, src->data, src->size, dest->data, available_blob(dest));
    if (size == 0) return -1;

    set_size_blob(dest, size);

    libdeflate_free_compressor(comp);

    return 0;
}

