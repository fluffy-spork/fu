#pragma once

// add '-lz' to jcc.env lib_flags


#include <zlib.h>


// NOTE(jason): overwrites src at the beginning
int
encode_gzip(blob_t * dest, const blob_t * src)
{
    // https://stackoverflow.com/a/57699371
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = src->size;
    zs.next_in = src->data;
    zs.avail_out = dest->capacity;
    zs.next_out = dest->data;

    // hard to believe they don't have a macro for gzip encoding, "Add 16" is
    // the best thing zlib can do: "Add 16 to windowBits to write a simple gzip
    // header and trailer around the compressed data instead of a zlib wrapper"
    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    int ret = deflate(&zs, Z_FINISH);
    deflateEnd(&zs);

    if (Z_STREAM_END != ret) {
        return -1;
    }

    set_size_blob(dest, zs.total_out);
    return 0;
}

