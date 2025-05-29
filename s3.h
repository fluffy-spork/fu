#pragma once

#include "sha256.h"

typedef struct {
    blob_t * url;
    blob_t * base_path;
    // includes port
    blob_t * host;
    blob_t * region;
    blob_t * access_key;
    blob_t * secret_key;
} s3_t;


s3_t *
new_s3(const blob_t * url)
{
    s3_t * s3 = malloc(sizeof(*s3));

    s3->url = blob(1024);
    s3->base_path = blob(1024);
    s3->host = blob(255);
    s3->region = blob(64);
    s3->access_key = blob(64);
    s3->secret_key = blob(64);

    add_blob(s3->url, url);

    ssize_t offset = 0;
    offset = index_blob_blob(url, B("://"), offset);
    offset += 3;
    scan_delim_blob(s3->host, url, B("/?"), &offset);
    if (offset_blob(url, offset - 1) == '/') {
        sub_blob(s3->base_path, url, offset - 1, -1);
//        debug_blob(s3->base_path);
    }

    return s3;
}


s3_t *
new_default_s3(void)
{
    blob_t * url = stk_blob(255);
    blob_env_fu(B("S3_URL"), url, NULL);

    s3_t * s3 = new_s3(url);

    // XXX: error checking
    blob_env_fu(B("S3_REGION"), s3->region, NULL);
    blob_env_fu(B("S3_ACCESS_KEY"), s3->access_key, NULL);
    blob_env_fu(B("S3_SECRET_KEY"), s3->secret_key, NULL);

//    debug_blob(s3->host);

    return s3;
}


u8
_char_to_upper_hex_s3(u8 c) {
    //assert(c < 16);
    return (c > 9) ? (c - 10) + 'A' : c + '0';
}

ssize_t
_write_upper_hex_s3(blob_t * b, void * src, ssize_t n_src)
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
        hex[j] = _char_to_upper_hex_s3((c & 0xf0) >> 4);
        hex[j + 1] = _char_to_upper_hex_s3(c & 0x0f);
    }

    return write_blob(b, hex, n_hex);
}


ssize_t
_uri_encode_s3(blob_t * dest, const blob_t * src, bool keep_slash)
{
    if (src->size == 0) return 0;

    for (ssize_t i = 0; i < (ssize_t)src->size; i++) {
        u8 c = src->data[i];

        if ((c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9')
            || c == '-'
            || c == '.'
            || c == '_'
            || c == '~'
            || (c == '/' && keep_slash))
        {
            write_blob(dest, &c, 1);
        } else {
            write_blob(dest, "%", 1);
            _write_upper_hex_s3(dest, &c, 1);
        }
    }

    return 0;
}


time_t
time_s3(void)
{
    return time(NULL);
}


time_t
utc_time_s3(int year, int mon, int day, int hour, int min, int sec)
{
    struct tm tm = {
        .tm_year = year - 1900,
        .tm_mon = mon - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
        .tm_isdst = -1
    };

    return timegm(&tm);
}


error_t
date_iso8601_s3(blob_t * b, time_t time)
{
    struct tm tm;
    gmtime_r(&time, &tm);

    add_s64_blob(b, tm.tm_year + 1900);
    add_s64_zero_pad_blob(b, tm.tm_mon + 1, 2);
    add_s64_zero_pad_blob(b, tm.tm_mday, 2);

    return 0;
}


error_t
datetime_iso8601_s3(blob_t * b, time_t time)
{
    struct tm tm;
    gmtime_r(&time, &tm);

    add_s64_blob(b, tm.tm_year + 1900);
    add_s64_zero_pad_blob(b, tm.tm_mon + 1, 2);
    add_s64_zero_pad_blob(b, tm.tm_mday, 2);
    write_blob(b, "T", 1);
    add_s64_zero_pad_blob(b, tm.tm_hour, 2);
    add_s64_zero_pad_blob(b, tm.tm_min, 2);
    add_s64_zero_pad_blob(b, tm.tm_sec, 2);
    write_blob(b, "Z", 1);

    return 0;
}


error_t
start_param_s3(blob_t * query, const blob_t * name)
{
    if (query->size > 0) {
        write_blob(query, "&", 1);
    }
    add_blob(query, name);
    write_blob(query, "=", 1);
    return query->error;
}


error_t
param_s3(blob_t * query, const blob_t * name, const blob_t * value)
{
    start_param_s3(query, name);
    add_blob(query, value);
    return query->error;
}


error_t
sign_url_s3(blob_t * signed_url, const blob_t * path, method_http_t method, int expires, time_t time, const s3_t * s3)
{
    time_t now = time == -1 ? time_s3() : time;

    blob_t * signed_headers = B("host");

    blob_t * credentials = stk_blob(255);
    add_blob(credentials, s3->access_key);
    write_blob(credentials, "/", 1);
    date_iso8601_s3(credentials, now);
    write_blob(credentials, "/", 1);
    add_blob(credentials, s3->region);
    write_blob(credentials, "/s3/", 4);
    add_blob(credentials, B("aws4_request"));

    blob_t * req = stk_blob(2048);
    add_line_blob(req, blob_method_http(method));

    blob_t * full_path = stk_blob(2048);
    add_blob(full_path, s3->base_path);
    add_path_file_fu(full_path, path);
//    debug_blob(full_path);

    _uri_encode_s3(req, full_path, true);
    newline_blob(req);

    // Canonical Query String
    //
    // X-Amz-Algorithm=AWS4-HMAC-SHA256&
    // X-Amz-Credential=AKIAIOSFODNN7EXAMPLE%2F20130524%2Fus-east-1%2Fs3%2Faws4_request&
    // X-Amz-Date=20130524T000000Z&
    // X-Amz-Expires=86400&
    // X-Amz-SignedHeaders=host
    blob_t * query = stk_blob(2048);

    param_s3(query, B("X-Amz-Algorithm"), B("AWS4-HMAC-SHA256"));
    start_param_s3(query, B("X-Amz-Credential"));
    _uri_encode_s3(query, credentials, false);
    start_param_s3(query, B("X-Amz-Date"));
    datetime_iso8601_s3(query, now);
    start_param_s3(query, B("X-Amz-Expires"));
    add_s64_blob(query, expires);
    param_s3(query, B("X-Amz-SignedHeaders"), signed_headers);

    add_line_blob(req, query);

    add_blob(req, B("host:"));
    add_line_blob(req, s3->host);
    // blank line after headers
    newline_blob(req);

    // signed headers
    add_line_blob(req, signed_headers);

    add_blob(req, B("UNSIGNED-PAYLOAD"));

//    debug_blob(req);

    blob_t * digest_req = stk_blob(32);
    sha256(digest_req, req);

    blob_t * hex_digest = stk_blob(64);
    write_hex_blob_blob(hex_digest, digest_req);

    blob_t * to_sign = stk_blob(1024);
    add_line_blob(to_sign, B("AWS4-HMAC-SHA256"));
    datetime_iso8601_s3(to_sign, now);
    newline_blob(to_sign);

    date_iso8601_s3(to_sign, now);
    write_blob(to_sign, "/", 1);
    add_blob(to_sign, s3->region);
    add_blob(to_sign, B("/s3/aws4_request"));
    newline_blob(to_sign);

    add_blob(to_sign, hex_digest);

//    debug_blob(to_sign);

    blob_t * key = stk_blob(256);
    blob_t * date_key = stk_blob(32);
    blob_t * date_region_key = stk_blob(date_key->capacity);
    blob_t * date_region_service_key = stk_blob(date_key->capacity);
    blob_t * signing_key = stk_blob(date_key->capacity);

    blob_t * date = stk_blob(8);
    date_iso8601_s3(date, now);

    add_blob(key,  B("AWS4"));
    add_blob(key, s3->secret_key);
    hmac_sha256(date_key, key, date);
    hmac_sha256(date_region_key, date_key, s3->region);
    hmac_sha256(date_region_service_key, date_region_key, B("s3"));
    hmac_sha256(signing_key, date_region_service_key, B("aws4_request"));

    blob_t * hmac = stk_blob(32);
    if (hmac_sha256(hmac, signing_key, to_sign)) {
        return -1;
    }

    blob_t * hex_hmac = stk_blob(64);
    write_hex_blob_blob(hex_hmac, hmac);

//    debug_blob(hex_hmac);

    add_blob(signed_url, s3->url);
    add_path_file_fu(signed_url, path);
    write_blob(signed_url, "?", 1);
    add_blob(signed_url, query);
    start_param_s3(signed_url, B("X-Amz-Signature"));
    add_blob(signed_url, hex_hmac);

//    debug_blob(signed_url);

    return 0;
}


error_t
sign_cacheable_url_s3(blob_t * signed_url, const blob_t * path, method_http_t method, int expires, const s3_t * s3)
{
    time_t now = time_s3();
    now = now - (now % expires);
    return sign_url_s3(signed_url, path, method, expires, now, s3);
}


error_t
head_s3(const blob_t * s3_path, const s3_t * s3)
{
    blob_t * signed_url = stk_blob(2048);
    sign_url_s3(signed_url, s3_path, head_method_http, 3600, -1, s3);

    return head_http(signed_url, NULL);
}


// worm_s3 (write once, read many)
error_t
put_file_s3(const blob_t * s3_path, const blob_t * local_path, const blob_t * content_type, cache_control_http_t cache_control, const s3_t * s3)
{
    blob_t * signed_url = stk_blob(2048);
    sign_url_s3(signed_url, s3_path, put_method_http, 3600, -1, s3);

    if (put_file_http(signed_url, local_path, content_type, cache_control, NULL)) {
        error_log("s3 file upload failed", "web", 1);
        return -1;
    }

    return 0;
}


error_t
get_file_s3(const blob_t * s3_path, const blob_t * local_path, const s3_t * s3)
{
    blob_t * signed_url = stk_blob(2048);
    sign_url_s3(signed_url, s3_path, get_method_http, 3600, -1, s3);

    if (get_file_http(signed_url, local_path, NULL)) {
        error_log("s3 file download failed", "web", 1);
        return -1;
    }

    return 0;
}


error_t
test_aws_s3()
{
    s3_t * s3 = new_s3(B("https://examplebucket.s3.amazonaws.com"));
    add_blob(s3->region, B("us-east-1"));
    add_blob(s3->access_key, B("AKIAIOSFODNN7EXAMPLE"));
    add_blob(s3->secret_key, B("wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"));

    time_t now = utc_time_s3(2013, 5, 24, 0, 0, 0);

    blob_t * signed_url = stk_blob(2048);
    sign_url_s3(signed_url, B("/test.txt"), get_method_http, 86400, now, s3);
    debug_blob(signed_url);

    return 0;
}

