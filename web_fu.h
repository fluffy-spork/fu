#pragma once

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <netinet/tcp.h>

#include "file_fu.h"

#define MAX_REQUEST_BODY 4096
#define MAX_RESPONSE_BODY 256*1024

#define MIN_RESOLUTION 480
#define X2_RESOLUTION (2*MIN_RESOLUTION)
#define X4_RESOLUTION (4*MIN_RESOLUTION)
#define MAX_SIZE_FILE_UPLOAD 100*1024*1024

#define OK_FILE_STATUS 0
#define ENCODE_FILE_STATUS 1
#define PROCESSING_FILE_STATUS 2
#define ENCODE_ERROR_FILE_STATUS 3
#define DELETE_FILE_STATUS 4
// NOTE(jason): maybe should be renamed.  the file row has been created,
// waiting for the file to be uploaded
#define NEW_FILE_STATUS 5

// TODO(jason): possibly should call this http_server (seems too long), httpd
// (daemon?), or something.  doesn't really matter and "web" is short.
// should possibly combine with http client methods

// TODO(jason): this should probably be a config in the db
#define COOKIE_EXPIRES_WEB (86400 * 365)

#define METHOD(var, E) \
    E(none, "NONE", var) \
    E(get, "GET", var) \
    E(post, "POST", var) \
    E(head, "HEAD", var) \
    E(put, "PUT", var) \

ENUM_BLOB(method, METHOD)

#define CONTENT_TYPE(var, E) \
    E(none, "none", var) \
    E(binary, "application/octet-stream", var) \
    E(urlencoded, "application/x-www-form-urlencoded", var) \
    E(multipart_form_data, "multipart/form-data", var) \
    E(text, "text/plain", var) \
    E(html, "text/html", var) \
    E(css, "text/css", var) \
    E(javascript, "text/javascript", var) \
    E(png, "image/png", var) \
    E(jpeg, "image/jpeg", var) \
    E(gif, "image/gif", var) \
    E(mp4, "video/mp4", var) \
    E(mov, "video/quicktime", var) \
    E(m3u8, "application/vnd.apple.mpegurl", var) \
    E(ts, "video/mp2t", var) \
    E(image_any, "image/*", var) \
    E(video_any, "video/*", var) \
    E(webp, "image/webp", var) \
    E(json, "application/json", var) \
    E(mpg, "video/mpeg", var) \
    E(sqlite, "application/vnd.sqlite3", var) \

ENUM_BLOB(content_type, CONTENT_TYPE)

typedef enum {
    UNKNOWN_CACHE_CONTROL,
    NO_STORE_CACHE_CONTROL,
    STATIC_ASSET_CACHE_CONTROL,
    USER_FILE_CACHE_CONTROL
} cache_control_t;

// NOTE(jason): the reason strings are pointless.  maybe need to store the code
// as a number, but this is only used for generating responses.
// how can  payload too large max size be configurable? max size should just be
// header
// update redirect to separate 302 found and 303 see other since they have
// different usages.
#define HTTP_STATUS(var, E) \
    E(unknown, "HTTP/1.1 000 N\r\n", var) \
    E(ok, "HTTP/1.1 200 X\r\n", var) \
    E(created, "HTTP/1.1 201 X\r\n", var) \
    E(not_found, "HTTP/1.1 404 X\r\n", var) \
    E(redirect, "HTTP/1.1 302 X\r\n", var) \
    E(permanent_redirect, "HTTP/1.1 301 X\r\n", var) \
    E(method_not_implemented, "HTTP/1.1 501 X\r\n", var) \
    E(bad_request, "HTTP/1.1 400 bad request\r\n", var) \
    E(internal_server_error, "HTTP/1.1 500 X\r\n", var) \
    E(service_unavailable, "HTTP/1.1 503 X\r\n", var) \
    E(payload_too_large, "HTTP/1.1 413 file too large: max size 100MB\r\n", var) \
    E(expect_continue, "100 X\r\n", var) \
    E(payment_required, "HTTP/1.1 402 X\r\n", var) \

ENUM_BLOB(http_status, HTTP_STATUS)

// TODO(jason): add init function?  all blobs would be correct size
typedef struct {
    blob_t * hostname;
    blob_t * base_url;
    s64 port;
    blob_t * res_dir;
    blob_t * upload_dir;
    s3_t * s3;
    blob_t * ffmpeg_path;
    int n_workers;
    int n_dev_workers;
    int (* after_fork_f)();
} config_web_t;

typedef struct endpoint_s endpoint_t;

typedef struct request_s request_t;
typedef int (* request_task_f)(request_t *);

struct request_s {
    int fd;

    struct timespec received;

    // sqlite uses s64 for rowid so... in theory could use negative ids to mark
    // for deletion
    s64 session_id;
    blob_t * session_cookie;
    bool session_supported;

    s64 user_id;
    blob_t * user_alias;

    s64 cart_id;

    // TODO(jason): maybe anonymize/mask the ip
    // for right now, any IP blocking, etc can be done in haproxy since it's
    // being used for https
    // blob_t * client_ip;

    method_t method;
    blob_t * uri;

    content_type_t request_content_type;
    size_t request_content_length;
    bool keep_alive;
    bool expect_continue;

    blob_t * request_head;
    bool request_head_parsed;
    blob_t * request_body;

    // portion of uri after a matching wildcard route
    // for /res/fluffy.png and route "/res/*" it's fluffy.png
    blob_t * path;
    blob_t * query;

    content_type_t content_type;
    s64 content_length;
    cache_control_t cache_control;

    http_status_t status;

    //blob_t * status_line;
    blob_t * head;
    blob_t * body;

    request_task_f after_task;
    s64 id_task; // pass an id to the task, like file_id, or whatever

    endpoint_t * endpoint;
};

typedef int (* request_handler_func)(endpoint_t * ep, request_t *);

#define EXTRACT_AS_ENUM_ENDPOINT(path, name, title, handler, ...) name##_id_endpoint,
#define EXTRACT_AS_VAR_ENDPOINT(path, name, title, handler, ...) endpoint_t * name;

#define EXTRACT_AS_PROTOTYPE_ENDPOINT(path, name, title, handler, ...) \
    int handler(endpoint_t * ep, request_t * req); // add endpoint_t

#define EXTRACT_AS_INIT_ENDPOINT(path, name, title, handler, ...) \
    endpoints.name = endpoint(name##_id_endpoint, path, name, title, handler, __VA_ARGS__); \
    endpoints.list[name##_id_endpoint] = endpoints.name; \

typedef enum {
    ENDPOINT_TABLE(EXTRACT_AS_ENUM_ENDPOINT)

    N_ENDPOINTS
} endpoint_id_t;

struct endpoint_s {
    endpoint_id_t id;
    blob_t * path;
    bool prefix_match;
    blob_t * name;
    // TODO(jason): possibly should be description
    blob_t * title;
    request_handler_func handler;

    bool params_parsed;
    u8 n_params;
    param_t * params;
};

#define endpoint(id, path, name, title, handler, ...) _endpoint(id, const_blob(path), const_blob(#name), const_blob(title), handler, __VA_ARGS__, NULL);

// NOTE(jason): vargs are field_t * for the input fields which could be NULL
endpoint_t *
_endpoint(endpoint_id_t id, blob_t * path, blob_t * name, blob_t * title, request_handler_func handler, ...)
{
    endpoint_t * ep = malloc(sizeof(*ep));
    if (!ep) return NULL;

    ep->id = id;
    ep->prefix_match = ends_with_char_blob(path, '*');
    if (ep->prefix_match) path->size--;
    ep->path = path;
    ep->name = name;
    ep->title = title;
    ep->handler = handler;

    u8 n = 0;
    field_t * fields[256];

    va_list args;
    va_start(args, handler);
    field_t * f;
    while ((f = va_arg(args, field_t *)))
    {
        fields[n] = f;
        n++;
    }
    va_end(args);

    param_t * params = calloc(n, sizeof(param_t));

    for (u8 i = 0; i < n; i++) {
        param_t * p = &params[i];
        init_param(p, fields[i]);
        /*
        p->field = fields[i];
        p->value = blob(fields[i]->max_size);
        p->error = blob(256);
        */

        //log_field(p->field);
    }

    ep->params_parsed = false;
    ep->params = params;
    ep->n_params = n;

    return ep;
}

struct {
    ENDPOINT_TABLE(EXTRACT_AS_VAR_ENDPOINT)

    size_t n_list;
    endpoint_t * list[N_ENDPOINTS];
} endpoints;

// NOTE(jason): catch if the handler hasn't been defined since this will fail
// during compile/link.
// Required since the handlers are defined in main.c
ENDPOINT_TABLE(EXTRACT_AS_PROTOTYPE_ENDPOINT)

void
init_endpoints()
{
    ENDPOINT_TABLE(EXTRACT_AS_INIT_ENDPOINT)

    endpoints.n_list = N_ENDPOINTS;
}

// returns the endpoint or null if id is out of range
endpoint_t *
by_id_endpoint(endpoint_id_t id)
{
    if (id < 0 || id >= endpoints.n_list) {
        error_log("invalid endpoint_id_t", __func__, id);
        return NULL;
    }

    return endpoints.list[id];
}

void
log_endpoint(endpoint_t * ep)
{
    log_var_u64(ep->id);
    log_var_blob(ep->name);
    log_var_blob(ep->title);
    log_var_u64(ep->n_params);

    for (u8 i = 0; i < ep->n_params; i++) {
        log_var_field(ep->params[i].field);
    }
}

// where to load resources
const blob_t * res_dir_web;
// where file uploads are stored defaults to "uploads" with "uploads/<randomid>"
const blob_t * upload_dir_web;
const s3_t * s3_web;

const blob_t * content_security_policy_web;

const blob_t * ffmpeg_path_web;

#define RES_WEB(var, E) \
    E(space," ", var) \
    E(hash,"#", var) \
    E(crlf,"\r\n", var) \
    E(crlfcrlf,"\r\n\r\n", var) \
    E(backslash,"/", var) \
    E(header_sep,":", var) \
    E(http_11,"HTTP/1.1 ", var) \
    E(ok,"ok", var) \
    E(ok_health,"ok health", var) \
    E(service_unavailable,"service unavailable", var) \
    E(internal_server_error,"internal server error", var) \
    E(payload_too_large,"payload too large", var) \
    E(see_other,"see other", var) \
    E(connection,"connection", var) \
    E(expect,"expect", var) \
    E(expect_100_continue,"100-continue", var) \
    E(connection_close,"close", var) \
    E(connection_keep_alive,"keep-alive", var) \
    E(content_type,"content-type", var) \
    E(content_length,"content-length", var) \
    E(content_disposition,"content-disposition", var) \
    E(content_disposition_value_prefix,"attachment; filename=", var) \
    E(content_encoding,"content-encoding", var) \
    E(content_encoding_gzip,"gzip", var) \
    E(content_encoding_brotli,"br", var) \
    E(content_encoding_identity,"identity", var) \
    E(cookie,"cookie", var) \
    E(set_cookie,"set-cookie", var) \
    E(location,"location", var) \
    E(x_frame_options,"x-frame-options", var) \
    E(deny_x_frame_options,"DENY", var) \
    E(content_security_policy,"content-security-policy", var) \
    E(transfer_encoding,"transfer-encoding", var) \
    E(chunked_transfer_encoding,"chunked", var) \
    E(end_chunk_transfer_encoding,"0\r\n\r\n", var) \
    E(name_session_cookie,"z", var) \
    E(session_cookie_attributes,"; path=/; max-age=15552000; httponly; samesite=lax", var) \
    E(secure_session_cookie_attributes,"; path=/; max-age=15552000; httponly; samesite=lax; secure", var) \
    E(method_not_implemented,"method not implemented", var) \
    E(bad_request,"bad request", var) \
    E(not_found,"not found", var) \
    E(payment_required, "payment required", var) \
    E(cache_control,"cache-control", var) \
    E(no_store,"no-store", var) \
    E(static_asset_cache_control,"public,max-age=2592000,immutable", var) \
    E(user_file_cache_control,"private,max-age=2592000,immutable", var) \
    E(res_path,"/res/", var) \
    E(res_path_suffix,"?v=", var) \
    E(favicon_path, "/res/logo.png", var) \
    E(file_table,"file_web", var) \
    E(files_dir,"files", var) \
    E(poster_kind,"poster", var) \
    E(dot_txt,".txt", var) \
    E(dot_html,".html", var) \
    E(dot_css,".css", var) \
    E(dot_js,".js", var) \
    E(dot_gif,".gif", var) \
    E(dot_jpg,".jpg", var) \
    E(dot_png,".png", var) \
    E(dot_mp4,".mp4", var) \
    E(dot_m3u8,".m3u8", var) \
    E(dot_ts,".ts", var) \
    E(access_log_table, "access_log_web", var) \
    E(dot_webp,".webp", var) \
    E(app_png, "app.png", var) \
    E(dot_mpg,".mpg", var) \
    E(dot_mov,".mov", var) \


ENUM_BLOB(res_web, RES_WEB)


u8
hex_char(u8 c)
{
    return ((c >= '0' && c <= '9') || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
        ? c
        : 0;
}

int
insert_access_log_web(request_t * req)
{
    s64 elapsed = elapsed_ns(req->received);
    s64 received = req->received.tv_sec;

    return insert_fields_db(app.db, res_web.access_log_table, NULL
            , request_method_field, req->method
            , request_path_field, req->path
            , http_status_field, (s64)req->status
            , session_id_field, req->session_id
            , user_id_field, req->user_id
            , received_field, received
            , elapsed_ns_field, elapsed
            , request_content_length_field, req->request_content_length
            , response_content_length_field, req->content_length
            );

}


ssize_t
percent_decode_blob(blob_t * dest, const blob_t * src)
{
    if (src->size == 0) return 0;

    // while there are more characters to scan and no error
    ssize_t offset = 0;
    while (scan_blob(dest, src, '%', &offset) != -1) {
        ssize_t remaining = remaining_blob(src, offset);

        if (remaining >= 2
                && hex_char(src->data[offset])
                && hex_char(src->data[offset + 1]))
        {
            //scan_hex_blob(dest, src, poffset);

            char hex[3] = { src->data[offset], src->data[offset + 1], 0 };
            u8 c = (u8)strtol(hex, NULL, 16);
            write_blob(dest, &c, 1);

            offset += 2;
        } else if (remaining > 0) {
            // remaining is 0 if it was end of blob
            write_blob(dest, "%", 1);
        }
    }

    // end of src.  what to do????
    return 0;
}

// NOTE(jason): I think this is correct, but the spec blows
ssize_t
percent_encode_blob(blob_t * dest, const blob_t * src)
{
    if (src->size == 0) return 0;

    for (ssize_t i = 0; i < (ssize_t)src->size; i++) {
        u8 c = src->data[i];
        if (isalnum(c) || c == '*' || c == '-' || c == '.' || c == '_') {
            write_blob(dest, &c, 1);
        } else {
            write_blob(dest, "%", 1);
            write_hex_blob(dest, &c, 1);
        }
    }

    return 0;
}

int
urlencode_field(blob_t * out, field_t * field, blob_t * value)
{
    add_blob(out, field->name);
    write_blob(out, "=", 1);
    percent_encode_blob(out, value);

    return 0;
}

int
urlencode_params(blob_t * out, param_t * params, size_t n_params)
{
    if (n_params == 0) return 0;

    write_blob(out, "?", 1);

    for (size_t i = 0; i < n_params; i++) {
        if (i > 0) write_blob(out, "&", 1);
        urlencode_field(out, params[i].field, params[i].value);
    }

    return 0;
}

// XXX(jason): not possible to tell if a parameter has an empty value, but
// I'm not sure if it matters.  easy enough to add a value even if it's
// ignored
// TODO(jason): should this trim the values of whitespace?  seems generally
// desirable, but not sure if it would cause developer confusion
int
urldecode_params(const blob_t * input, param_t * params, size_t n_params)
{
    assert(input != NULL);

    if (empty_blob(input)) return 0;

    //debug_blob(input);

    assert(params != NULL);
    assert(n_params > 0);

    blob_t * name = stk_blob(255);
    blob_t * urlencoded = stk_blob(1024);

    int count = 0;

    ssize_t offset = 0;

    // a '&' at the beginning would be included in whichever param name is
    // first which is wrong
    skip_u8_blob(input, '&', &offset);

    // XXX(jason): not correct parsing
    while (scan_blob(urlencoded, input, '=', &offset) != -1) {
        reset_blob(name);
        percent_decode_blob(name, urlencoded);

        //debug_blob(name);

        // NOTE(jason): read the value here because the offset has to be moved
        // past it regardless
        reset_blob(urlencoded);
        scan_blob(urlencoded, input, '&', &offset);

        for (size_t i = 0; i < n_params; i++) {
            param_t param = params[i];
            field_t * field = param.field;

            if (equal_blob(field->name, name)) {
                // XXX(jason); what should be done if value is too small?
                replace_blob(urlencoded, '+', ' ');
                percent_decode_blob(param.value, urlencoded);
                //debug_blob(param.value);
                count++;
            }
        }

        reset_blob(urlencoded);
    }

    return count;
}

void
clear_params(param_t * params, size_t n_params)
{
    for (size_t i = 0; i < n_params; i++) {
        reset_blob(params[i].value);
        reset_blob(params[i].error);
    }
}

content_type_t
content_type_magic(const blob_t * data)
{
    // XXX: this could all be way better.
    //
    // really need to fix this to remove wrap_array and get this into the
    // content type enum
    static char jpeg_magic[] = { 0xFF, 0xD8, 0xFF };
    static char webp_magic[] = { 0x57, 0x45, 0x42, 0x50, 0x56, 0x50, 0x38 };
    static char png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    static char gif_magic[] = { 0x47, 0x49, 0x46, 0x38 };
    //static char mp4_magic[] = { 0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32 };
    static char mp4_magic[] = { 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D };
    static char mp42_magic[] = { 0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32 };
    static char mov_magic[] = { 0x66, 0x74, 0x79, 0x70 };

    blob_t * magic = stk_blob(40);
    sub_blob(magic, data, 0, magic->capacity);

    if (begins_with_blob(data, wrap_array_blob(jpeg_magic))) {
        return jpeg_content_type;
    }
    else if (begins_with_blob(data, wrap_array_blob(png_magic))) {
        return png_content_type;
    }
    else if (begins_with_blob(data, wrap_array_blob(gif_magic))) {
        return gif_content_type;
    }
    else if (first_contains_blob(data, wrap_array_blob(webp_magic))) {
        return webp_content_type;
    }
    else if (first_contains_blob(data, wrap_array_blob(mp4_magic))) {
        return mp4_content_type;
    }
    else if (first_contains_blob(data, wrap_array_blob(mp42_magic))) {
        return mp4_content_type;
    }
    else if (first_contains_blob(data, wrap_array_blob(mov_magic))) {
        return mov_content_type;
    }

    return binary_content_type;
}

content_type_t
content_type_path(const blob_t * path)
{
    if (ends_with_blob(path, res_web.dot_css)) {
        return css_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_js)) {
        return javascript_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_png)) {
        return png_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_jpg)) {
        return jpeg_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_webp)) {
        return webp_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_gif)) {
        return gif_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_txt)) {
        return text_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_m3u8)) {
        return m3u8_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_ts)) {
        return ts_content_type;
    }
    else if (ends_with_blob(path, res_web.dot_mpg)) {
        return mpg_content_type;
    }

    // if there's no extension, attempt to identify based on magic number
    int fd = open_read_file_fu(path);
    if (fd == -1) {
        log_errno("open_read_file_fu");
        return binary_content_type;
    }

    blob_t * magic = stk_blob(255);
    if (read_file_fu(fd, magic) == -1) {
        log_errno("read_file_fu");
        return binary_content_type;
    }

    return content_type_magic(magic);
}

const blob_t *
suffix_content_type(content_type_t type)
{
    switch (type) {
        case jpeg_content_type:
            return res_web.dot_jpg;
        case mp4_content_type:
            return res_web.dot_mp4;
        case mov_content_type:
            return res_web.dot_mov;
        case webp_content_type:
            return res_web.dot_webp;
        case png_content_type:
            return res_web.dot_png;
        case gif_content_type:
            return res_web.dot_gif;
        case m3u8_content_type:
            return res_web.dot_m3u8;
        case ts_content_type:
            return res_web.dot_ts;
        default:
            log_var_s64(type);
            log_var_blob(blob_content_type(type));
            dev_error("invalid content type");
    }
}

bool
video_content_type(content_type_t type)
{
    switch (type) {
        case mp4_content_type:
        case mov_content_type:
        case video_any_content_type:
            return true;
        default:
            return false;
    }
}

bool
image_content_type(content_type_t type)
{
    switch (type) {
        case jpeg_content_type:
        case png_content_type:
        case gif_content_type:
        case webp_content_type:
        case image_any_content_type:
            return true;
        default:
            return false;
    }
}


content_type_t
preferred_image_type(content_type_t type)
{
    if (type == webp_content_type) {
        return jpeg_content_type;
    }

    if (video_content_type(type)) {
        return mp4_content_type;
    }

    return type;
}


bool
post_request(request_t * req)
{
    return req->method == post_method;
}

// do whatever's necessary to erase/clean a request to be reused for a
// different request.
// would really prefer the whole request to be memset(0) on a single memory
// block.
// maybe this should be erase_request
void
reuse_request(request_t * req)
{
    req->fd = -1;
    req->method = none_method;
    req->content_type = none_content_type;
    req->content_length = -1;
    req->cache_control = NO_STORE_CACHE_CONTROL;
    req->request_content_type = none_content_type;
    req->request_content_length = 0;
    req->keep_alive = true;
    req->expect_continue = false;

    req->session_id = 0;
    req->session_supported = false;
    req->user_id = 0;
    erase_blob(req->user_alias);

    req->cart_id = 0;

    req->status = 0;

    req->after_task = NULL;
    req->id_task = 0;

    erase_blob(req->uri);
    erase_blob(req->path);
    erase_blob(req->query);
    erase_blob(req->head);
    erase_blob(req->body);
    erase_blob(req->request_head);
    req->request_head_parsed = false;
    erase_blob(req->request_body);
    erase_blob(req->session_cookie);

    req->endpoint = NULL;
}

request_t *
new_request()
{
    request_t * req = malloc(sizeof(request_t));
    if (!req) return NULL;

    // decide if these sizes matter
    req->uri = blob(512);
    req->path = blob(512);
    req->query = blob(512);

    req->request_head = blob(2048);
    req->request_body = blob(MAX_REQUEST_BODY);

    req->session_cookie = blob(256);
    // TODO(jason): how big should this actually be?  should be fairly short
    // and limited so page layout can be consistent
    req->user_alias = blob(32);

    req->head = blob(1024);
    req->body = blob(MAX_RESPONSE_BODY);

    reuse_request(req);

    return req;
}

void
header(blob_t * t, const blob_t *name, const blob_t *value)
{
    vadd_blob(t, name, res_web.header_sep, value, res_web.crlf);
}


void
header_request_web(request_t * req, const blob_t * name, const blob_t * value)
{
    header(req->head, name, value);
}


void
s64_header_request_web(request_t * req, const blob_t * name, const s64 value)
{
    blob_t * b = req->head;

    vadd_blob(b, name, res_web.header_sep);
    add_s64_blob(b, value);
    add_blob(b, res_web.crlf);
}


// TODO(jason): probably remove and add s64_header
void
u64_header(blob_t * b, const blob_t *name, const u64 value)
{
    vadd_blob(b, name, res_web.header_sep);
    add_u64_blob(b, value);
    add_blob(b, res_web.crlf);
}

// Makes the response download potentially with save as dialog
//Content-Disposition: attachment; filename="cool.html"
//
// TODO(jason): why doesn't this take a request_t?
void
add_content_disposition_header(blob_t * b, const blob_t * filename)
{
    blob_t * v = stk_blob(256);
    vadd_blob(v, res_web.content_disposition_value_prefix, filename);
    header(b, res_web.content_disposition, v);
}


void
gzip_content_encoding_web(request_t * req)
{
    header(req->head, res_web.content_encoding, res_web.content_encoding_gzip);
}


void
static_cache_control_web(request_t * req)
{
    req->cache_control = STATIC_ASSET_CACHE_CONTROL;
}


// TODO(jason): this probably shouldn't be "require"
int
require_request_head_web(request_t * req)
{
    // NOTE(jason): no headers or headers already processed
    if (req->request_head_parsed) return 0;

    blob_t * headers = req->request_head;
    //debug_blob(headers);

    blob_t * name = stk_blob(255);
    blob_t * value = stk_blob(1024);

    ssize_t offset = 0;

    while (scan_blob(name, headers, ':', &offset) != -1) {
        skip_whitespace_blob(headers, &offset);
        // probably could skip this here and just do it at each value below?
        scan_blob(value, headers, '\n', &offset);
        trim_tail_blob(value);

        if (case_equal_blob(res_web.content_type, name)) {
            req->request_content_type = enum_content_type(value, 0);
        }
        else if (case_equal_blob(res_web.content_length, name)) {
            req->request_content_length = s64_blob(value, 0);
        }
        else if (case_equal_blob(res_web.connection, name)) {
            // TODO(jason): keep alive is not close so...
            req->keep_alive = equal_blob(value, res_web.connection_keep_alive)
                || !equal_blob(value, res_web.connection_close);
        }
        else if (case_equal_blob(res_web.cookie, name)) {
            blob_t * name_cookie = stk_blob(255);
            blob_t * value_cookie = stk_blob(255);

            ssize_t coff = 0;
            while (scan_blob(name_cookie, value, '=', &coff) != -1) {
                if (equal_blob(name_cookie, res_web.name_session_cookie)) {
                    if (scan_blob(value_cookie, value, ';', &coff) != -1) {
                        add_blob(req->session_cookie, value_cookie);
                        req->session_supported = true;
                        break;
                    }
                }
            }
        }
        else if (case_equal_blob(res_web.expect, name)) {
            // NOTE(jason): supposedly expect header is only used by curl and
            // browsers don't
            req->expect_continue = case_equal_blob(res_web.expect_100_continue, value);
        }

        reset_blob(name);
        reset_blob(value);
    }

    if (req->expect_continue) {
        // NOTE(jason): curl supposedly only sends expect when the POST body is
        // larger and then hangs if the connection isn't closed.
        req->keep_alive = false;
    }

    req->request_head_parsed = true;

    return 0;
}

// TODO(jason): possibly should include a blob to read the body into and
// replace read_request_body
int
require_request_body_web(request_t * req)
{
    require_request_head_web(req);

    //debug_blob(req->request_body);

    if (req->request_content_length > req->request_body->size) {
        //info_log("request content length larger than intial read");
        // TODO(jason): this should be better and check that the full
        // content-length can be read.  although, the general case should be
        // that it fits in request_body unless it's a file upload
        read_file_fu(req->fd, req->request_body);
    }

    return 0;
}

int
query_params(request_t * req, endpoint_t * ep)
{
    //debug_blob(req->query);

    if (ep->params_parsed) return 0;

    if (empty_blob(req->query)) return 0;

    ep->params_parsed = true;
    return urldecode_params(req->query, ep->params, ep->n_params);
}

int
post_params(request_t * req, endpoint_t * ep)
{
    if (ep->params_parsed) return 0;

    dev_error(req->method != post_method);

    // NOTE(jason): has to be called before next check so the headers have been
    // read
    require_request_head_web(req);

    dev_error(req->request_content_type != urlencoded_content_type);

    require_request_body_web(req);
    ep->params_parsed = true;
    return urldecode_params(req->request_body, ep->params, ep->n_params);
}

int
request_params(request_t * req, endpoint_t * ep)
{
    if (post_request(req)) {
        return post_params(req, ep);
    }
    else {
        return query_params(req, ep);
    }
}

/*
void
status_line(request_t * req)
{
    blob_t * b = req->status_line;

    add_blob(b, res_web.http_11);
    vadd_blob(b, req->status, res_web.crlf);
}
*/

// TODO(jason): possibly confusing that this includes the blank line.  it means
// extra headers have to be added before calling thie function
void
add_response_headers(request_t *req)
{
    blob_t * t = req->head;

    if (req->content_type != none_content_type) {
        header(t, res_web.content_type, blob_content_type(req->content_type));
    }

    // NOTE(jason): content length is always required because of persistent
    // connections/keep alive.  this was a problem with webkit and redirects
    // would hang
    if (req->content_length != -1) {
        u64_header(t, res_web.content_length, req->content_length);
    }
    else {
        req->content_length = req->body->size;
        u64_header(t, res_web.content_length, (u64)req->body->size);
    }

    if (req->cache_control == STATIC_ASSET_CACHE_CONTROL) {
        header(t, res_web.cache_control, res_web.static_asset_cache_control);
    }
    else if (req->cache_control == USER_FILE_CACHE_CONTROL) {
        header(t, res_web.cache_control, res_web.user_file_cache_control);
    }
    else {
        header(t, res_web.cache_control, res_web.no_store);
    }

    if (!req->keep_alive) {
        header(t, res_web.connection, res_web.connection_close);
    }

    // NOTE(jason): it's fucking stupid that deny isn't the default and every
    // request is required to have this.
    header(t, res_web.x_frame_options, res_web.deny_x_frame_options);
    header(t, res_web.content_security_policy, content_security_policy_web);

    // NOTE(jason): this is the blank line to indicate the headers are over and
    // the body should be next
    add_blob(t, res_web.crlf);
}

int
ok_response(request_t * req, content_type_t type)
{
    req->content_type = type;
    req->status = ok_http_status;

    return 0;
}

int
created_response(request_t * req, content_type_t type, blob_t * location)
{
    req->content_type = type;
    req->status = created_http_status;

    header(req->head, res_web.location, location);

    return 0;
}

// TODO(jason): maybe this should be a macro that sets the blob_t * html.
int
html_response(request_t * req)
{
    return ok_response(req, html_content_type);
}

int
text_response(request_t * req)
{
    return ok_response(req, text_content_type);
}


// TODO(jason): maybe this should be a macro that sets the blob_t * html.
int
json_response(request_t * req)
{
    return ok_response(req, json_content_type);
}


int
css_response(request_t * req)
{
    return ok_response(req, css_content_type);
}


int
javascript_response(request_t * req)
{
    return ok_response(req, javascript_content_type);
}


// generate an error response that formats a body with html about the error
// TODO(jason): make a macro to pass file and line of caller
int
error_response(request_t * req, http_status_t status, blob_t * reason)
{
    // NOTE(jason): have to overwrite existing head and body in error
    // case and not just append to any existing code.
    reset_blob(req->head);
    reset_blob(req->body);

    // TODO(jason): if req content type is json, then output json instead of
    // html
    req->content_type = html_content_type;
    req->status = status;

    blob_t * html = req->body;

    page_begin(reason);
    h1(reason);
    page_end();

    // needs to include the request path
    error_log(cstr_blob(reason), "http", status);

    return -1;
}

int
internal_server_error_response(request_t * req)
{
    // TODO(jason): should probably have the status code and message as a
    // struct.  maybe a typedef of a general integer/message struct
    return error_response(req, internal_server_error_http_status, res_web.internal_server_error);
}

int
redirect_web(request_t * req, const blob_t * url)
{
    // TODO(jason): 303 may not actually work.  it's technically correct by the standard,
    // but 302 has always been used for redirecting with a GET after a POST
    req->status = redirect_http_status;

    // TODO(jason): this likely needs to be an absolute url.  relative works
    // fine in chrome, but curl doesn't seem like it's working correctly.  May
    // not matter
    header(req->head, res_web.location, url);

    return 0;
}


// 301
// used with /favicon.ico to allow caching of the redirect
int
permanent_redirect_web(request_t * req, const blob_t * url)
{
    req->cache_control = STATIC_ASSET_CACHE_CONTROL;
    req->status = permanent_redirect_http_status;

    // TODO(jason): this likely needs to be an absolute url.  relative works
    // fine in chrome, but curl doesn't seem like it's working correctly.  May
    // not matter
    header(req->head, res_web.location, url);

    return 0;
}

int
method_not_implemented_response(request_t * req)
{
    // TODO(jason): should probably have the status code and message as a
    // struct.  maybe a typedef of a general integer/message struct
    return error_response(req, method_not_implemented_http_status, res_web.method_not_implemented);
}

int
method_not_implemented_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);
    return method_not_implemented_response(req);
}

// TODO(jason): make these use the file, function, and line number
#define bad_request_response(req) error_response(req, bad_request_http_status, res_web.bad_request)

int
bad_request_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);
    return bad_request_response(req);
}

int
not_found_response(request_t * req)
{
    return error_response(req, not_found_http_status, res_web.not_found);
}

int
not_found_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);
    return not_found_response(req);
}

int
payment_required_response(request_t * req)
{
    error_response(req, payment_required_http_status, res_web.payment_required);

    return -1;
}

int
payment_required_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);
    return payment_required_response(req);
}

int
payload_too_large_response(request_t * req)
{
    return error_response(req, payload_too_large_http_status, res_web.payload_too_large);
}

// TODO(jason): add last-modified response header?  i'm not sure if it matters
// because all the static assets have super long cache control max-age so
// should never be checking.  probably be better to use etag/if-not-match
int
file_response(request_t * req, const blob_t * dir, const blob_t * path, content_type_t content_type, s64 user_id)
{
    blob_t * file_path = stk_blob(4096);
    path_file_fu(file_path, dir, path);

    int fd = open_read_file_fu(file_path);
    if (fd == -1) {
        not_found_response(req);
        return log_errno("open");
    }

    //debug_s64(fd);

    struct stat st;
    if (fstat(fd, &st) == -1) {
        internal_server_error_response(req);
        return log_errno("fstat");
    }

    size_t len = st.st_size;
    //debug_s64(len);

    if (content_type == 0) {
        content_type = content_type_path(file_path);
    }

    // TODO(jason): need to look at the actual data for content type
    ok_response(req, content_type);
    // TODO(jason): not sure if all file responses should be cacheable
    req->cache_control = user_id  ? USER_FILE_CACHE_CONTROL : STATIC_ASSET_CACHE_CONTROL;
    req->content_length = len;
    add_response_headers(req);

    const blob_t * status = blob_http_status(req->status);

    // NOTE(jason):  TCP_CORK supposedly helps wait to send everything with
    // full packets or something when there are headers and sendfile.  man sendfile
    // maybe that's important to avoid sending a few byte packet with the
    // status or headers before the whole file is sent
    set_tcp_cork_file(req->fd, 1);

    if (write_file_fu(req->fd, status) == -1) {
        internal_server_error_response(req);
        return log_errno("write_file_fu status");
    }

    if (write_file_fu(req->fd, req->head) == -1) {
        internal_server_error_response(req);
        return log_errno("write_file_fu head");
    }

    if (send_file(req->fd, fd, len) == -1) {
        internal_server_error_response(req);
        return log_errno("send_file");
    }

    set_tcp_cork_file(req->fd, 0);

    return 1;
}

int
url_params_endpoint(blob_t * url, endpoint_t * ep, param_t * params, size_t n_params)
{
    add_blob(url, ep->path);
    urlencode_params(url, params, n_params);

    return 0;
}

int
redirect_params_endpoint(request_t * req, const endpoint_t * ep, param_t * params, size_t n_params)
{
    blob_t * url = stk_blob(255);
    add_blob(url, ep->path);
    urlencode_params(url, params, n_params);

    return redirect_web(req, url);
}

int
url_field_endpoint(blob_t * url, endpoint_t * ep, field_t * field, blob_t * value)
{
    add_blob(url, ep->path);
    write_blob(url, "?", 1);
    urlencode_field(url, field, value);

    return 0;
}

bool
match_route_endpoint(endpoint_t * ep, request_t * req)
{
    if (ep->prefix_match) {
        return begins_with_blob(req->path, ep->path);
    } else {
        return equal_blob(req->path, ep->path);
    }
}

int
route_endpoint(request_t * req)
{
    for (int i = endpoints.n_list - 1; i >= 0; i--) {
        endpoint_t * ep = endpoints.list[i];
        if (match_route_endpoint(ep, req)) {
            //log_endpoint(ep);
            // TODO(jason): make sure parameters are cleared so don't get used
            // by a later request.  this doesn't seem very good.  maybe
            // endpoint should be on the request and clear params in
            // reuse_request
            ep->params_parsed = false;
            clear_params(ep->params, ep->n_params);
            req->endpoint = ep;
            return ep->handler(ep, req);
        }
    }

    return 0;
}

// find the param within the endpoint fields with the field_id
param_t *
param_endpoint(endpoint_t * ep, field_id_t field_id)
{
    return by_id_param(ep->params, ep->n_params, field_id);
}

blob_t *
set_param_endpoint(endpoint_t * ep, field_id_t field_id, blob_t * value)
{
    param_t * p = param_endpoint(ep, field_id);
    assert(p != NULL);
    set_blob(p->value, value);
    return value;
}

s64
set_s64_param_endpoint(endpoint_t * ep, field_id_t field_id, s64 value)
{
    param_t * p = param_endpoint(ep, field_id);
    assert(p != NULL);
    set_s64_blob(p->value, value);
    return value;
}

// TODO(jason): doesn't seem like the greatest solution since there's no way to
// return an error.  probably doesn't matter as the onl possible error is the
// param not existing.
// what should return be?
s64
set_s64_optional_param_endpoint(endpoint_t * ep, field_id_t field_id, s64 value)
{
    param_t * p = param_endpoint(ep, field_id);
    if (p) {
        set_s64_blob(p->value, value);
        return value;
    }

    return value;
}

// Sets the param value if it's empty
// TODO(jason): this does weird things if called before the params have been
// parsed.  it may be that the solution is to always parse params before
// calling handlers?
s64
default_s64_param_endpoint(endpoint_t * ep, field_id_t field_id, s64 value)
{
    // TODO(jason): this could be better
    param_t * p = param_endpoint(ep, field_id);
    assert(p != NULL);
    if (empty_blob(p->value) || (p->value->size == 1 && p->value->data[0] == '0')) {
        reset_blob(p->value);
        add_s64_blob(p->value, value);
    }

    return s64_blob(p->value, 0);
}

// Sets the param value if it's empty
int
default_param_endpoint(endpoint_t * ep, field_id_t field_id, blob_t * value)
{
    param_t * p = param_endpoint(ep, field_id);
    assert(p != NULL);
    if (empty_blob(p->value)) {
        add_blob(p->value, value);
    }

    return 0;
}

// NOTE(jason): this is probably fine in most cases as long as it's ok for dest
// to have extra params and it doesn't matter if they're in the url
int
redirect_endpoint(request_t * req, const endpoint_t * dest, endpoint_t * src)
{
    return src != NULL
        ? redirect_params_endpoint(req, dest, src->params, src->n_params)
        : redirect_web(req, dest->path);
}

int
redirect_next_endpoint(request_t * req, endpoint_t * ep, endpoint_t * default_ep, blob_t * hash)
{
    assert_not_null(default_ep);

    param_t * next_id = param_endpoint(ep, next_id_field);
    if (!next_id || empty_blob(next_id->value)) {
        return redirect_endpoint(req, default_ep, NULL);
    }

    // NOTE(jason): have to make sure the url is a relative url
    if (next_id->value->data[0] != '/') {
        return redirect_endpoint(req, default_ep, NULL);
    }

    if (valid_blob(hash)) {
        blob_t * u = stk_blob(next_id->value->size + 2*hash->size);
        vadd_blob(u, next_id->value, res_web.hash, hash);
        return redirect_web(req, u);
    }
    else {
        return redirect_web(req, next_id->value);
    }
}

// TODO(jason): re-evaluate if this flow is correct.  possibly make all sqlite errors an assert/abort?
// this create seems odd i don't think it's quite right.  the only place that
// sessions seem to need to be created is the email_login_code_handler.
// everywhere else is only read.  until maybe there's a shopping cart or
// something.  that could also directly create a session if necessary too.
//
// TODO(jason): I don't like this name and create parameter.
// require_session_web should always create as "require".  but another function
// can be added like optional_session_web that doesn't create.  Also removes
// the blind true/false parameter in calls.  see optional_user_web
//
// TODO(jason): is another random value cookie needed that's only sent to the
// login path or something that helps security?  i'm not sure what i'm talking about.
int
_session_web(request_t * req, bool create)
{
    if (req->session_id) return 0;

    require_request_head_web(req);

    //debug_blob(req->session_cookie);

    if (valid_blob(req->session_cookie)) {
        sqlite3_stmt * stmt;

        if (prepare_db(app.db, &stmt, B("select session_id, u.user_id, u.alias from session_web s left join user u on u.user_id = s.user_id where s.cookie = ? and s.expires > unixepoch()"))) {
            internal_server_error_response(req);
            return -1;
        }

        text_bind_db(stmt, 1, req->session_cookie);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            req->session_id = s64_db(stmt, 0);
            req->user_id = s64_db(stmt, 1);
            if (req->user_id) blob_db(stmt, 2, req->user_alias);

            //log_var_s64(req->session_id);
            //log_var_s64(req->user_id);
            //log_var_blob(req->user_alias);
        }

        if (finalize_db(stmt) != SQLITE_OK) {
            internal_server_error_response(req);
            return -1;
        }

        // update last access time?  I don't think it matters and there should
        // just be a max time session expiration.  it should be in logs anyway

        if (req->session_id) return 0;
    }

    if (create) {
        s64 new_id = random_s64_fu();

        blob_t * cookie = stk_blob(32);
        add_random_blob(cookie, 16);

        //log_var_blob(cookie);

        sqlite3_stmt * stmt;
        if (prepare_db(app.db, &stmt, B("insert into session_web (session_id, created, modified, expires, cookie) values (?, unixepoch(), unixepoch(), unixepoch() + ?, ?)"))) {
            internal_server_error_response(req);
            return -1;
        }
        s64_bind_db(stmt, 1, new_id);
        s64_bind_db(stmt, 2, COOKIE_EXPIRES_WEB);
        text_bind_db(stmt, 3, cookie);
        sqlite3_step(stmt);
        if (finalize_db(stmt)) {
            internal_server_error_response(req);
            return -1;
        }

        req->session_id = new_id;
        set_blob(req->session_cookie, cookie);

        blob_t * c = stk_blob(255);

        add_blob(c, res_web.name_session_cookie);
        write_blob(c, "=", 1);
        add_blob(c, cookie);
        //write_hex_blob(c, &new_id, sizeof(new_id));
        // TODO(jason): I keep getting bitten by this setting secure when it's
        // not or vice versa, but the real fix is implementing ssl directly in
        // fluffy.  The other option is actually checking for x-forwarded-proto
        // header and setting a flag, but ideally it shouldn't be necessary
        if (dev_mode()) {
            add_blob(c, res_web.session_cookie_attributes);
        }
        else {
            add_blob(c, res_web.secure_session_cookie_attributes);
        }
        header(req->head, res_web.set_cookie, c);
    }

    return 0;
}

int
require_session_web(request_t * req)
{
    int ret = _session_web(req, true);
    return (ret == 0 && req->session_id > 0) ? 0 : -1;
}

int
optional_session_web(request_t * req)
{
    return _session_web(req, false);
}

int
end_session_web(request_t * req)
{
    if (!req->session_id) return 0;

    // NOTE(jason): set session id cookie to blank?  probably doesn't matter as
    // it will be invalid the next request anyway.

    blob_t * sql = B("update session_web set expires = unixepoch(), modified = unixepoch() where session_id = ?1");
    return exec_s64_pi_db(app.db, sql, NULL, req->session_id);
}

int
_set_redirect_url_web(request_t * req)
{
    dev_error(req->session_id == 0);

    return exec_s64_pbi_db(app.db, B("update session_web set redirect_url = ?, modified = unixepoch() where session_id = ?"),
            NULL, req->uri, req->session_id);
}

int
require_user_web(request_t * req, const endpoint_t * auth)
{
    if (require_session_web(req)) return -1;

    if (req->user_id) return 0;

    if (!auth) {
        not_found_response(req);
        return -1;
    }

    if (_set_redirect_url_web(req)) {
        internal_server_error_response(req);
    }
    else {
        redirect_endpoint(req, auth, NULL);
    }

    return 1;
}

// requires a session and if no user sets redirect url for after login
int
optional_user_web(request_t * req)
{
    if (require_session_web(req)) return -1;

    if (req->user_id) return 0;

    if (_set_redirect_url_web(req)) {
        return internal_server_error_response(req);
    }

    return 0;
}

// return a file from the AppDir/res directory with cache-control header set
// for static assets.  the url should include some sort of cache breaker query
// parameter.  there will be a function for generating res urls.
int
res_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);

    return file_response(req, res_dir_web, req->path, 0, 0);
}


void
css_web(blob_t * css, const blob_t * fg, const blob_t * bg, const blob_t * fg_highlight, const blob_t * bg_highlight)
{
    const blob_t * bg_error_color = B("#c00");
    const blob_t * fg_error_color = B("#fff");
    const blob_t * disabled_color = B("#666");
    const blob_t * none = B("none");
    const blob_t * zero = B("0");
    const blob_t * center = B("center");

    reset_css(css);

    begin_css(css, B("body"));
    {
        background_color_css(css, bg);
        color_css(css, fg);
        font_family_css(css, B("monospace"));
        font_size_css(css, B("16px"));
        font_weight_css(css, B("bold"));
    }
    end_css(css);

    begin_css(css, B("::selection"));
    {
        background_color_css(css, fg_highlight);
        color_css(css, bg_highlight);
        text_shadow_css(css, none);
    }
    end_css(css);

    begin_css(css, B("table"));
    {
        margin_css(css, B("1em auto"));
    }
    end_css(css);

    begin_css(css, B("tr:nth-child(even)"));
    {
        // TODO(jason): configurable?
        background_color_css(css, B("#222"));
    }
    end_css(css);

    begin_css(css, B("th, td"));
    {
        padding_css(css, B("4px 8px"));
        text_wrap_css(css, B("nowrap"));
    }
    end_css(css);

    begin_css(css, B("option:hover, option:focus"));
    {
        background_color_css(css, bg_highlight);
        color_css(css, fg_highlight);
    }
    end_css(css);

    begin_css(css, B("h1"));
    {
        margin_css(css, B("1em 0"));
        text_align_css(css, center);
        font_size_css(css, B("24px"));
    }
    end_css(css);

    begin_css(css, B("video"));
    {
        cursor_css(css, B("pointer"));
        display_css(css, B("block"));
        // for safari overlays
        transform_css(css, B("translateZ(0)"));
        prop_css(css, B("-webkit-tap-highlight-color"), B("rgba(0, 255, 0, 0.25)"));
    }
    end_css(css);

    link_css(css, fg_highlight, bg_highlight);

    begin_css(css, B("form"));
    {
        text_align_css(css, center);
    }
    end_css(css);

    begin_css(css, B("input"));
    {
        display_css(css, B("inline-block"));
        appearance_css(css, none);
        font_size_css(css, B("18px"));
        font_family_css(css, B("monospace"));
        padding_css(css, B("0.5em"));
    }
    end_css(css);

    begin_css(css, B("textarea, select, input[type=text]"));
    {
        outline_css(css, none);
        display_css(css, B("block"));
        appearance_css(css, none);
        width_css(css, B("100%"));
        box_sizing_css(css, B("border-box"));
        border_css(css, B("2px solid"));
        border_color_css(css, bg);
        padding_css(css, B("0.5em"));
        overflow_css(css, B("clip"));
        background_color_css(css, bg);
        // NOTE(jason): the input text as fg (white) seems better.  should
        // there be a specific input color?
        color_css(css, fg);
        font_size_css(css, B("18px"));
        font_family_css(css, B("monospace"));
        font_weight_css(css, B("bold"));
    }
    end_css(css);

    begin_css(css, B("textarea"));
    {
        font_size_css(css, B("20px"));
    }
    end_css(css);

    begin_css(css, B("textarea:focus, textarea:hover, input:focus, input:hover, select:focus, select:hover"));
    {
        border_css(css, B("2px dashed"));
        border_color_css(css, fg_highlight);
    }
    end_css(css);

    class_css(css, B("notice"));
    {
        background_color_css(css, bg_error_color);
        color_css(css, fg_error_color);
    }
    end_css(css);

    class_css(css, B("field"));
    {
        margin_bottom_css(css, B("16px"));
        padding_css(css, B("8px 0 0 8px"));
        border_css(css, none);
        border_left_css(css, B("2px solid"));
        border_top_css(css, B("2px solid"));
        border_color_css(css, fg_highlight);
        text_align_css(css, B("left"));
    }
    end_css(css);

    class_css(css, B("field > label"));
    {
        font_size_css(css, B("12px"));
        font_weight_css(css, B("normal"));
        color_css(css, fg_highlight);
    }
    end_css(css);

    id_css(css, B("header"));
    {
        background_color_css(css, bg);
        text_align_css(css, center);
    }
    end_css(css);

    id_css(css, B("user"));
    {
        margin_css(css, B("32px"));
        text_align_css(css, center);
    }
    end_css(css);

    id_css(css, B("main"));
    {
        margin_css(css, B("1em auto 0 auto"));
        padding_css(css, B("0 0 128px 0"));
        background_color_css(css, bg);

        width_css(css, B("100%"));
        min_width_css(css, B("256px"));
        max_width_css(css, B("640px"));
        min_height_css(css, B("256px"));
    }
    end_css(css);

    class_css(css, center);
    {
        text_align_css(css, center);
    }
    end_css(css);

    class_css(css, B("error"));
    {
        background_color_css(css, bg_error_color);
        color_css(css, fg_error_color);
    }
    end_css(css);

    class_css(css, B("errors"));
    {
        margin_css(css, B("16px"));
        text_align_css(css, B("start"));
    }
    end_css(css);

    begin_css(css, B(".errors > .error"));
    {
        padding_css(css, B("8px"));
        border_radius_css(css, B("8px"));
    }
    end_css(css);

    class_css(css, B("action-bar"));
    {
        position_css(css, B("sticky"));
        z_index_css(css, B("1000"));
        top_css(css, zero);
        margin_css(css, B("1em 0"));
        text_align_css(css, B("right"));
    }
    end_css(css);

    begin_css(css, B(".action-bar .button-bar"));
    {
        box_shadow_css(css, B("0 0 2px 0 #000"));
        background_color_css(css, B("rgba(0,0,0,0.4)"));
    }
    end_css(css);

    begin_css(css, B("form"));
    {
        text_align_css(css, center);
    }
    end_css(css);

    media_css(css, B("screen and (min-width: 720px)"));
    {
        class_css(css, B("action-bar"));
        {
            position_css(css, B("fixed"));
            top_css(css, zero);
            right_css(css, zero);
        }
        end_css(css);

        begin_css(css, B(".action-bar > .button-bar"));
        {
            border_bottom_left_radius_css(css, B("12px"));
            flex_direction_css(css, B("column"));
        }
        end_css(css);

        begin_css(css, B(".action-bar > .button-bar > *"));
        {
            margin_css(css, B("8px"));
        }
        end_css(css);
    }
    end_media_css(css);

    class_css(css, B("button-bar"));
    {
        pointer_events_css(css, none);
        display_css(css, B("flex"));
        flex_wrap_css(css, B("wrap"));
        padding_css(css, zero);
        justify_content_css(css, center);
    }
    end_css(css);

    begin_css(css, B("table.tri"));
    {
        margin_css(css, zero);
        width_css(css, B("100%"));
        border_spacing_css(css, zero);
        border_collapse_css(css, B("collapse"));
    }
    end_css(css);

    begin_css(css, B("table.tri td"));
    {
        display_css(css, B("table-cell"));
        padding_css(css, zero);
        width_css(css, B("33%"));
        text_align_css(css, center);
        vertical_align_css(css, B("middle"));
        white_space_css(css, B("nowrap"));
    }
    end_css(css);

    begin_css(css, B("table.tri td:first-child"));
    {
        text_align_css(css, B("left"));
    }
    end_css(css);

    begin_css(css, B("table.tri td:last-child"));
    {
        text_align_css(css, B("right"));
    }
    end_css(css);

    begin_css(css, B("input[type=submit], button, .button-bar a"));
    {
        cursor_css(css, B("pointer"));
        user_select_css(css, none);
        display_css(css, B("inline-block"));
        outline_css(css, none);
        margin_css(css, B("8px"));
        padding_css(css, B("8px"));
        border_css(css, B("2px solid"));
        border_color_css(css, fg_highlight);
        border_radius_css(css, B("8px"));
        background_color_css(css, B("rgba(0,0,0,0.4)"));
        color_css(css, fg_highlight);
        font_size_css(css, B("18px"));
        font_weight_css(css, B("bold"));
        text_shadow_css(css, B("0 0 2px #000"));
        box_shadow_css(css, B("0 0 2px 0 #000"));
    }
    end_css(css);

    begin_css(css, B(".button-bar input, .button-bar button, .button-bar a"));
    {
        pointer_events_css(css, B("auto"));
    }
    end_css(css);

    begin_css(css, B("input[type=submit]:focus, input[type=submit]:hover, button:focus, button:hover, .button-bar a:focus, .button-bar a:hover"));
    {
        border_style_css(css, B("dashed"));
        text_decoration_css(css, none);
    }
    end_css(css);

    begin_css(css, B("input[type=submit]:active, button:active, .button-bar a:active"));
    {
        background_color_css(css, fg_highlight);
        border_style_css(css, B("solid"));
        color_css(css, bg);
        text_shadow_css(css, none);
    }
    end_css(css);

    begin_css(css, B("input[type=submit]:disabled, button:disabled, .button-bar a:disabled"));
    {
        background_color_css(css, bg);
        border_color_css(css, disabled_color);
        color_css(css, disabled_color);
    }
    end_css(css);

    begin_css(css, B(".button-bar form, .user form"));
    {
        display_css(css, B("inline-block"));
    }
    end_css(css);

    begin_css(css, B("form > .button-bar"));
    {
        position_css(css, B("sticky"));
        top_css(css, zero);
        flex_wrap_css(css, B("nowrap"));
        margin_bottom_css(css, B("1em"));
        box_shadow_css(css, B("0 0 2px 0 #000"));
        background_color_css(css, B("rgba(0,0,0,0.4)"));
    }
    end_css(css);

    begin_css(css, B(".button-bar .status"));
    {
        flex_grow_css(css, B("1"));
        padding_css(css, B("8px"));
        line_height_css(css, B("200%"));
        x_overflow_css(css, B("clip"));
        text_overflow_css(css, B("ellipsis"));
        white_space_css(css, B("nowrap"));
    }
    end_css(css);

    class_css(css, B("primary-action"));
    {
        pointer_events_css(css, none);
        position_css(css, B("absolute"));
        top_css(css, zero);
        left_css(css, zero);
        right_css(css, zero);
        text_align_css(css, B("right"));
    }
    end_css(css);

    begin_css(css, B(".primary-action input[type=submit]"));
    {
        pointer_events_css(css, B("auto"));
    }
    end_css(css);

    begin_css(css, B(".primary-action form"));
    {
        display_css(css, B("inline-block"));
        margin_css(css, B("4px 8px"));
    }
    end_css(css);

    begin_css(css, B("#home-login input[name=email]"));
    {
        margin_css(css, B("3em auto"));
        //padding_css(css, zero);
        text_align_css(css, center);
        width_css(css, B("32ex"));
    }
    end_css(css);

    begin_css(css, B(".big, .amount"));
    {
        margin_css(css, B("0 0 1em 0"));
        text_align_css(css, center);
        font_size_css(css, B("24px"));
    }
    end_css(css);

    begin_css(css, B(".big > .button-bar"));
    {
        flex_wrap_css(css, B("wrap"));
    }
    end_css(css);

    begin_css(css, B(".item-list > *"));
    {
        display_css(css, B("block"));
        margin_css(css, B("0.25em 0"));
        font_size_css(css, B("32px"));
    }
    end_css(css);
}


// TODO(jason): this works.  ultimately, I think it's unfortunately required to
// have something at /favicon.ico
int
favicon_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);

    // Use the AppImage AppDir app.png as the favicon
    // may need to change later if we need different resolutions
    return file_response(req, app.dir, res_web.app_png, 0, 0);

    //return permanent_redirect_web(req, res_web.favicon_path);
}

// XXX(jason): since not targeting using a load balancer by default this
// probably isn't necessary anymore.
int
health_handler(endpoint_t * ep, request_t * req)
{
    // minimum number of non-crashed workers?  Needs a load component too, but
    // maybe the compute engine cpu load works
    static bool healthy_web = true;

    UNUSED(ep);

    if (healthy_web) {
        error_response(req, ok_http_status, res_web.ok_health);
    } else {
        error_response(req, service_unavailable_http_status, res_web.service_unavailable);
    }

    return 0;
}


int
insert_file_web(db_t * db, s64 * file_id, s64 user_id, blob_t * path, s64 size, s64 content_type)
{
    // TODO(jason): is NEW_FILE_STATUS a good name?  this is called before the
    // file has actually been uploaded
    return insert_fields_db(db, res_web.file_table, file_id,
            user_id_field, user_id,
            path_field, path,
            size_field, size,
            content_type_field, content_type,
            status_field, (s64)NEW_FILE_STATUS);
}


int
set_status_file(db_t * db, s64 file_id, int file_status)
{
    def_param(status);
    add_s64_blob(status.value, file_status);

    return update_params(db, res_web.file_table, &status, 1, fields.file_id, file_id, 0);
}

int
delete_file_web(db_t * db, s64 file_id)
{
    return set_status_file(db, file_id, DELETE_FILE_STATUS);
}

int
file_info_web(db_t * db, param_t * file_id, param_t * path, param_t * content_type)
{
    return by_id_db(db, B("select path, content_type from file_web where file_id = ?"), s64_blob(file_id->value, 0), path, content_type);
}

#define MAX_CMD_WEB 1024

int
path_upload_web(blob_t * path, const blob_t * name)
{
    return path_file_fu(path, upload_dir_web, name);
}

// TODO(jason): don't like calling an external prog.  eventually replace with header code
int
_ffmpeg_call_web(const blob_t * input, const blob_t * output, const blob_t * filter)
{
//    char * fmt_cmd = "%s -hide_banner -loglevel error -nostdin -i %s %s %s";

    blob_t * cmd = stk_blob(2048);
    add_blob(cmd, ffmpeg_path_web);
    add_cstr_blob(cmd, " -hide_banner -loglevel error -nostdin -i ");
    quote_add_blob(cmd, input);
    write_blob(cmd, " ", 1);
    add_blob(cmd, filter);
    write_blob(cmd, " ", 1);
    quote_add_blob(cmd, output);

    /*
    char cmd[MAX_CMD_WEB];
    if (snprintf(cmd, MAX_CMD_WEB, fmt_cmd, cstr_blob(ffmpeg_path_web), cstr_blob(input), cstr_blob(filter), cstr_blob(output)) < 0) {
        log_errno("ffmpeg cmd format failed");
        return -1;
    }
    */

    // NOTE(jason): excessive logging
    //info_log(cmd);

//    debug_blob(cmd);

    // TODO(jason): XXX really need to stop use system and a shell
    int rc = system(cstr_blob(cmd));
    if (rc) {
        log_errno("failed to process media file");
        return -1;
    }

    return 0;
}

int
resize_ffmpeg_web(const blob_t * input, const blob_t * output, s64 width)
{
    blob_t * filter = stk_blob(1024);
    add_blob(filter, B("-y -vf scale="));
    add_s64_blob(filter, width);
    add_blob(filter, B(":-1"));

    log_var_blob(filter);

    return _ffmpeg_call_web(input, output, filter);
}

int
resize_webp_web(const blob_t * input, const blob_t * output, s64 width)
{
    blob_t * filter = stk_blob(1024);
    add_blob(filter, B("-y -vf scale="));
    add_s64_blob(filter, width);
    add_blob(filter, B(":-1"));

    log_var_blob(filter);

    return _ffmpeg_call_web(input, output, filter);
}

// TODO(jason): don't like calling an external prog.  eventually replace with header code
int
resize_jpeg_web(const blob_t * input, const blob_t * output, s64 width)
{
    // TODO(jason): this doesn't work for some jpegs I have.  possibly
    // switching to webp for everything anyway so not going to research why
    // ffmpeg doesn't work.
    // at least one of the problems is orientation data getting lost and the
    // image being the wrong orientation
    // I think there was also an issue with reading some file
    //return resize_ffmpeg_web(input, output, width);

    char cmd[MAX_CMD_WEB];

    // TODO(jason): revisit stripping exif data to verify
    if (width > 0) {
#ifdef __APPLE__
        char * fmt_cmd = "sips --resampleWidth %d %s --out %s";
#else
        char * fmt_cmd = "convert-im6 -auto-orient +profile exif -resize %dx %s %s";
#endif

        if (snprintf(cmd, MAX_CMD_WEB, fmt_cmd, width, cstr_blob(input), cstr_blob(output)) < 0) {
            log_errno("imagemagick convert cmd format failed");
            return -1;
        }
    }
    else {
        // NOTE(jason): this is for the width == 0 case mainly.  still want to
        // process the image and not just send out the uploaded version so that
        // eventually the exif data, etc is stripped.
#ifdef __APPLE__
        // sips doesn't do anything in this case
        //char * fmt_cmd = "sips %s --out %s";
        //still should strip exif
        char * fmt_cmd = "cp '%s' '%s'";
#else
        // TODO(jason): single quote in the filenames aren't escaped
        char * fmt_cmd = "convert-im6 -auto-orient +profile exif \'%s\' \'%s\'";
#endif

        if (snprintf(cmd, MAX_CMD_WEB, fmt_cmd, cstr_blob(input), cstr_blob(output)) < 0) {
            log_errno("imagemagick convert cmd format failed");
            return -1;
        }
    }

    info_log(cmd);

    int rc = system(cmd);
    if (rc) {
        log_errno("failed to process media file");
        return -1;
    }

    return 0;
}

int
resize_gif_web(const blob_t * input, const blob_t * output, s64 width)
{
    blob_t * filter = stk_blob(1024);
    add_blob(filter, B("-y -vf \"scale="));
    add_s64_blob(filter, width);
    add_blob(filter, B(":-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\" -loop 0"));

    log_var_blob(filter);

    return _ffmpeg_call_web(input, output, filter);
    //return resize_ffmpeg_web(input, output, width);
}

int
resize_png_web(const blob_t * input, const blob_t * output, s64 width)
{
    return resize_ffmpeg_web(input, output, width);
}

// NOTE(jason): poster is from 6 seconds into clip since there's currently 4
// seconds of padding.
int
extract_poster_web(const blob_t * input, const blob_t * output, s64 width)
{
    blob_t * filter = stk_blob(1024);
    add_blob(filter, B("-y -ss 6 -vframes 1 -vf scale="));
    add_s64_blob(filter, width);
    add_blob(filter, B(":-1"));

    log_var_blob(filter);

    return _ffmpeg_call_web(input, output, filter);
}

int
transcode_video_web(const blob_t * input, const blob_t * output, s64 width, const blob_t * watermark_file)
{
    blob_t * filter = stk_blob(1024);

    s64 crf = 23;
    s64 maxrate = 1024;
    s64 fpsmax = 0;
    bool watermark = false;

    // NOTE(jason): some of these values are from apple's author guide for HLS
    // https://developer.apple.com/documentation/http_live_streaming/http_live_streaming_hls_authoring_specification_for_apple_devices
    if (width == 0) {
        // highest quality at source resolution
        crf = 18;
        maxrate = 8192;
    }
    else if (width == MIN_RESOLUTION) {
        crf = 23;
        maxrate = 300;
        fpsmax = 30;
        watermark = true;
    }
    else if (width == X2_RESOLUTION) {
        crf = 23;
        maxrate = 3000;
        fpsmax = 30;
    }
    else if (width == X4_RESOLUTION) {
        crf = 23;
        maxrate = 6000;
    }

    s64 bufsize = maxrate;

    if (watermark) {
        add_blob(filter, B(" -i "));
        add_blob(filter, watermark_file);
    }

    add_blob(filter, B(" -y -filter_complex \""));
    add_blob(filter, B("scale="));
    add_s64_blob(filter, width);
    add_blob(filter, B(":-1"));
    if (watermark) {
        add_blob(filter, B(",overlay=(W-w)/2:((H-h)/2)"));
    }
    add_blob(filter, B("\""));

    // remove audio from video uploads
    // TODO(jason): probably make this a config option
    add_blob(filter, B(" -an"));
    add_blob(filter, B(" -c:v libx264 -preset medium -crf "));
    add_s64_blob(filter, crf);
    if (fpsmax) {
        add_blob(filter, B(" -fpsmax "));
        add_s64_blob(filter, fpsmax);
    }
    add_blob(filter, B(" -maxrate "));
    add_s64_blob(filter, maxrate);
    add_blob(filter, B("k -bufsize "));
    add_s64_blob(filter, bufsize);
    add_blob(filter, B("k -pix_fmt yuv420p -movflags +faststart"));

    log_var_blob(filter);

    return _ffmpeg_call_web(input, output, filter);
}


error_t
media_path_web(blob_t * path, s64 width, const blob_t * name, content_type_t type)
{
    add_s64_blob(path, width);
    add_path_file_fu(path, name);
    add_blob(path, suffix_content_type(type));

    return path->error;
}


// NOTE(jason): currently assuming
// std_width = 640
// hd_width = 2*std_width
// video default width = 1920
// image default width = ???
int
process_media_web(param_t * file_id, s64 width, content_type_t target_type)
{
    def_param(path);
    def_param(content_type);

    if (file_info_web(app.db, file_id, &path, &content_type)) {
        log_error_db(app.db, "failed to get file info");
        return -1;
    }

    content_type_t type = s64_blob(content_type.value, 0);
    if (target_type == none_content_type) {
        target_type = preferred_image_type(type);
    }

    blob_t * local_input = stk_blob(255);
    path_upload_web(local_input, path.value);
//    debug_blob(local_input);

    blob_t * s3_path = stk_blob(255);
    path_file_fu(s3_path, res_web.files_dir, path.value);
//    debug_blob(s3_path);

    if (read_access_file_fu(local_input)) {
        if (get_file_s3(s3_path, local_input, s3_web)) {
            error_log("s3 source download failed", "web", 1);
            // goto error;
            return -1;
        }
    }
    else if (head_s3(s3_path, s3_web)) {
//        debug("upload local file to s3");
        if (put_file_s3(s3_path, local_input, blob_content_type(type), user_immutable_cache_control_http, s3_web)) {
            error_log("s3 source upload failed", "web", 1);
        }
    }
    else {
//        debug("file already exists in s3");
    }

    // TODO(jason): this path building seems a little wonky
    blob_t * media_path = stk_blob(255);
    media_path_web(media_path, width, path.value, target_type);

    blob_t * dir = stk_blob(255);
    path_upload_web(dir, stk_s64_blob(width));
    ensure_dir_file_fu(dir, S_IRWXU);

    blob_t * output = stk_blob(255);
    path_upload_web(output, media_path);

    //debug_blob(input);
    //debug_blob(output);

    int rc = 0;

    if (video_content_type(type)) {
        if (target_type == jpeg_content_type) {
            //debug("XXXXX extract poster XXXX");
            rc = extract_poster_web(local_input, output, width);
        }
        else {
            if (process_media_web(file_id, width, jpeg_content_type)) {
                log_errno("failed to generate video poster");
            }

            blob_t * watermark_file = stk_blob(1024);
            path_file_fu(watermark_file, res_dir_web, B("/res/watermark.png"));

            //debug("XXXXX transcode video XXXX");
            rc = transcode_video_web(local_input, output, width, watermark_file);
        }
    }
    else if (gif_content_type == type) {
        //debug("XXXXX resize gif XXXX");
        rc = resize_gif_web(local_input, output, width);
    }
    else if (png_content_type == type) {
        //debug("XXXXX resize png XXXX");
        rc = resize_png_web(local_input, output, width);
    }
    else if (jpeg_content_type == type) {
        //debug("XXXXX resize jpg XXXX");
        // remove exif and other identifying info
        rc = resize_jpeg_web(local_input, output, width);
    }
    else if (webp_content_type == type) {
        //debug("XXXXX resize webp XXXX");
        // remove exif and other identifying info
        rc = resize_webp_web(local_input, output, width);
    }

    if (rc) return rc;

    blob_t * output_s3_path = stk_blob(255);
    path_file_fu(output_s3_path, res_web.files_dir, media_path);
    if (put_file_s3(output_s3_path, output, blob_content_type(target_type), user_immutable_cache_control_http, s3_web)) {
        error_log("s3 file upload failed", "web", 1);
        return -1;
    }

    return 0;
}


// minimal processing to fast as possible for during response
// full processing is done in the background
int
fast_process_media_web(param_t * file_id)
{
    return process_media_web(file_id, MIN_RESOLUTION, none_content_type);
}

int
full_process_media_web(s64 id)
{
    def_param(file_id);
    add_s64_blob(file_id.value, id);

    if (fast_process_media_web(&file_id)
            || process_media_web(&file_id, X2_RESOLUTION, none_content_type)
            || process_media_web(&file_id, 0, none_content_type))
    {
        return -1;
    }

    return 0;
}

// file_id is set to 0 when no files need encoding
int
next_process_media_web(s64 * file_id)
{
    *file_id = 0;

    blob_t * sql = B("update file_web set status = 2 where file_id = (select file_id from file_web where status = 1 and (select count(*) from file_web where status = 2) = 0) returning file_id");
    return exec_s64_db(app.db, sql, file_id) == 0 && *file_id > 0;
}

int
process_media_task_web(request_t * req)
{
    // TODO(jason): give priority to request uploaded file if not processing
    // backlog.  really just need to process faster in general.
    UNUSED(req);

    // process files as long as more to process and no other threads processing
    s64 file_id = 0;
//    debug("process media task");
    while (next_process_media_web(&file_id)) {
//        debug_s64(file_id);

        if (full_process_media_web(file_id)) {
            if (set_status_file(app.db, file_id, ENCODE_ERROR_FILE_STATUS)) {
                // assume everything is just borked.  crash??
                log_error_db(app.db, "failure reseting file to encode status");
                return -1;
            }

            continue;
        }

        if (set_status_file(app.db, file_id, 0)) {
            // assume everything is just borked.  crash??
            log_error_db(app.db, "failure setting file to 0 status");
            return -1;
        }

        flush_log();
    }

    return 0;
}

int
files_process_media_task_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);

    html_response(req);
    add_blob(req->body, B("ok"));

    req->after_task = process_media_task_web;
    req->keep_alive = false;

    return 0;
}


int
backup_db_task_web(request_t * req)
{
    // TODO(jason): give priority to request uploaded file if not processing
    // backlog.  really just need to process faster in general.
    UNUSED(req);

    return backup_main_db_app();
}


int
task_backup_db_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);

    html_response(req);
    add_blob(req->body, B("ok"));

    req->after_task = backup_db_task_web;
    req->keep_alive = false;

    return 0;
}


// TODO(jason): can still upload a file of an invalid/unknown file type
int
files_upload_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);

    // NOTE(jason): as long as this endpoint doesn't do a UI not much point in
    // redirecting.
    if (require_user_web(req, NULL)) return -1;

    if (req->request_content_length == 0) {
        log_var_s64(req->request_content_length);
        return bad_request_response(req);
    }

    if (req->request_content_length > MAX_SIZE_FILE_UPLOAD) {
        //return bad_request_response(req);
        return payload_too_large_response(req);
    }

    // should check user permissions before sending 100 continue
    if (req->expect_continue) {
        // TODO(jason): what to do if this write fails?
        if (write_file_fu(req->fd, http_status.expect_continue) == -1) {
            // NOTE(jason): will this even work if it's failed.  probably not
            // as the only real failure scenario is the socket won't write.
            return internal_server_error_response(req);
        }
        //check for read available on req->fd
    }

    if (empty_blob(req->request_body)) {
        if (read_file_fu(req->fd, req->request_body) == -1) {
            log_errno("read initial request body");
            return bad_request_response(req);
        }
    }

    content_type_t type = content_type_magic(req->request_body);
    if (type == binary_content_type) {
        // TODO(jason): this change doesn't seem right.  I'm not sure what's
        // going on?
        //
        // was this because of chrome ignoring accept jpg and sending webp?
        //type = mp4_content_type;
        log_var_s64(type);
        return bad_request_response(req);
    }

    blob_t * file_key = stk_blob(32);
    fill_random_blob(file_key);

    blob_t * path = stk_blob(255);
    path_upload_web(path, file_key);

    s64 file_id;
    if (insert_file_web(app.db, &file_id, req->user_id, file_key, req->request_content_length, type)) {
        log_error_db(app.db, "insert_file web");
        internal_server_error_response(req);
        return -1;
    }

    // TODO(jason): switch to storing the files with a SHA256 or something
    // filename to avoid duplicate uploads.
    // could potentially avoid uploads if the digest header is used with
    // requests or some other method to avoid even uploading dupes.
    if (write_prefix_file_fu(path, req->request_body, req->fd, req->request_content_length) == -1) {
        log_error_db(app.db, "write_prefix_file");
        internal_server_error_response(req);
        return -1;
    }

    if (image_content_type(type)) {
        full_process_media_web(file_id);
    }
    else if (video_content_type(type)) {
        param_t file_id_p = local_param(fields.file_id);
        add_s64_blob(file_id_p.value, file_id);

        set_status_file(app.db, file_id, ENCODE_FILE_STATUS);

        fast_process_media_web(&file_id_p);

        req->after_task = process_media_task_web;
        //req->id_task = file_id;
    }

    // NOTE(jason): need to set keep_alive before response headers are sent
    // so connection: close is sent.  without connection close, the browser
    // still hangs assuming it can make another request on the connection.
    // chrome wouldn't open a new connection which seems a little weird.
    // doesn't matter.
    req->keep_alive = false;

    blob_t * location = stk_blob(255);
    add_s64_blob(location, file_id);

    return created_response(req, binary_content_type, location);
}

int
files_handler(endpoint_t * ep, request_t * req)
{
    // NOTE(jason): as long as this endpoint doesn't do a UI not much point in
    // redirecting.
    if (require_user_web(req, NULL)) return -1;

    if (query_params(req, ep) == -1) {
        debug("query_params");
        return -1;
    }

    param_t * file_id = param_endpoint(ep, file_id_field);
    if (empty_blob(file_id->value)) {
        internal_server_error_response(req);
        return -1;
    }

    def_param(path);
    def_param(content_type);

    if (file_info_web(app.db, file_id, &path, &content_type)) {
        not_found_response(req);
        return -1;
    }

    content_type_t type = s64_blob(content_type.value, 0);

    param_t * kind = param_endpoint(ep, kind_field);
    if (equal_blob(kind->value, res_web.poster_kind)) {
        type = jpeg_content_type;
    }

    blob_t * filename = stk_blob(path.value->capacity + 4);
    vadd_blob(filename, path.value, suffix_content_type(type));

    //debug_blob(filename);

    return file_response(req, upload_dir_web, filename, type, req->user_id);

    /*
    int rc = file_response(req, upload_dir_web, filename, type, req->user_id);
    if (rc == -1 && errno == ENOENT) {
        if (process_media_web(file_id)) {
            return internal_server_error_response(req);
        }

        return file_response(req, upload_dir_web, filename, type, req->user_id);
    }

    return rc;
    */
}

int
handle_request(request_t *req)
{
    ssize_t result = 0;
    const size_t n_iov = 3;
    struct iovec iov[n_iov];

    // TODO(jason): adjust this size so the entire request is read one shot in
    // non-file upload cases.
    blob_t * input = stk_blob(MAX_REQUEST_BODY/2);
    blob_t * tmp = stk_blob(1024);

    // NOTE(jason): read in the request line, headers, part of body.  any body
    // will be copied to req->request_body and handlers can read the rest of
    // the body if necessary.
    //
    // If the request delays sending data this closes the connection.  for
    // example, if you're manually typing into telnet to test.  It doesn't
    // seem like a huge deal so far.  Reasonable users should be immediately
    // sending the request and not just opening a socket.  I'm wondering if
    // this will be a problem with shitty internet connections though.
    // Seemed to still work ok with chrome set to network throttle poor 3g
    //
    // I think not waiting for data is actually causing http keep alive to only
    // work when the request has already been sent.  may not be happening at
    // all as there are currently many "empty request" log messages.
    ssize_t n = read_file_fu(req->fd, input);
    if (n == -1) {
        if (errno != EAGAIN) { // not timeout
            log_errno("read request head");
        }
        return -1;
    } else if (n == 0) {
        // TODO(Jason): seems like this is happening way more than desirable.
        // although I guess it always has to happened for all requests that do
        // keep alive?  maybe it's just stupid to log.
        info_log("empty request");
        return -1;
    }

    // NOTE(jason): have to get the start time for a request after the request
    // line has been read.  Also, chrome will reconnect and not send a request
    // if the connection is closed without a "connection: close" header.  would
    // be required for supporting multiple requests on a connection too.
    timestamp_log(&req->received);

    //debug_blob(input);

    ssize_t offset = 0;

    // Request-Line = Method SP Request-URI SP HTTP-Version CRLF
    // Method is case-sensitive and uppercase

    if (scan_blob(tmp, input, ' ', &offset) == -1) {
        bad_request_response(req);

        goto respond;
    }

    req->method = enum_method(tmp, 0);
    if (req->method == none_method) {
        // NOTE(jason): this happens when using http and chrome gives the
        // warning page requiring a continue button. tmp is blank.  Probably
        // doesn't matter since https will always be used in production.
        method_not_implemented_response(req);

        goto respond;
    }

    reset_blob(tmp);

    // parse uri

    if (scan_blob(req->uri, input, ' ', &offset) == -1) {
        bad_request_response(req);

        goto respond;
    }

    split_blob(req->uri, '?', req->path, req->query);

    //log_var_blob(req->uri);
    //log_var_blob(req->path);
    //log_var_blob(req->query);

    // NOTE(jason): scan to the end of the line which should end in
    // "HTTP/1.XCRLF" which we don't care about.  Doesn't matter as long as it
    // isn't super long and nobody does something crazy to the standard in the
    // future

    // TODO(jason): why isn't this just skipping to the end and not writing into tmp
    if (scan_blob(tmp, input, '\n', &offset) == -1) {
        bad_request_response(req);

        goto respond;
    }

    //log_blob(tmp, "http version");

    // copy the headers from input to req->request_headers(?) and req->request_body

    if (scan_blob_blob(req->request_head, input, res_web.crlfcrlf, &offset) == -1) {
        log_var_s64(req->request_head->error);
        bad_request_response(req);

        goto respond;
    }

    // NOTE(jason): ignore for requests with an empty body
    if (remaining_blob(input, offset) > 0
            && sub_blob(req->request_body, input, offset, -1) == -1) {
        // NOTE(jason): I spent a lot of time on a bug caused by having body
        // smaller.  the real fix is input is a fraction of the
        // MAX_REQUEST_BODY size
        if (req->request_body->error == ENOSPC) {
            debug("request body somehow smaller than input size");
            dev_error(req->request_body->capacity < input->capacity);
        }

        internal_server_error_response(req);
        goto respond;
    }

    //debug_blob(req->request_head);
    //debug_blob(req->request_body);

    // NOTE(jason): don't parse headers here or otherwise process the body.
    // That way no time is wasted parsing headers, etc for a url that doesn't
    // even exist

    // TODO(jason): idk if i like this switch usage
    switch (route_endpoint(req)) {
        case -1:
            //debug("error routing endpoint keep alive false");
            req->keep_alive = false;
            break;
        case 0:
            break;
        default:
            // should be > 0
            // the handler already wrote the full response
            //elapsed_log(start, "request-time");
            insert_access_log_web(req);
            return 0;
    }

respond:
    // no response default to 404
    if (!req->status) {
        not_found_response(req);
    }

    if (req->body->error) {
        // TODO(jason); should this be a dev_error?
        error_log("response probably too big", "http", req->body->error);
        internal_server_error_response(req);
    }

    if (req->after_task) {
        // NOTE(jason): browser will hang until the after task is done if keep alive not disabled
        req->keep_alive = false;
    }

    // TODO(jason): handler should've set the status line by this point
    add_response_headers(req);

    const blob_t * status = blob_http_status(req->status);

    //debug_blob(status);
    //debug_blob(req->head);

    // write headers and body that should be in request_t
    // TODO(jason): maybe this should be done in the caller?
    iov[0].iov_base = status->data;
    iov[0].iov_len = status->size;
    iov[1].iov_base = req->head->data;
    iov[1].iov_len = req->head->size;
    iov[2].iov_base = req->body->data;
    iov[2].iov_len = req->body->size;
    result = writev(req->fd, iov, n_iov);

    //debug_s64(result);
    if (result == -1) log_errno("write response");

    // XXX: this shouldn't be here
    if (app.db && !sqlite3_get_autocommit(app.db)) {
        // NOTE(jason): the rollback is here just so the db won't be borked
        rollback_db(app.db);
        dev_error("sqlite transaction open");
    }

    insert_access_log_web(req);

    //elapsed_log(start, "request-time");

    return 0;
}

pid_t
fork_worker_web(int srv_fd, int (* after_fork_f)())
{
    pid_t pid = fork_app_fu();

    if (pid != 0) {
        return pid;
    }

    // worker
    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    request_t * req = new_request();

    if (!req) {
        log_errno("new_request");
        exit(EXIT_FAILURE);
    }

    // NOTE(jason): callback so application can open db connections,
    // etc
    if (after_fork_f) after_fork_f();

    flush_log();

    while (1)
    {
        int client_fd = accept(srv_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            log_errno("accept");
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        if (setsockopt (client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))
            || setsockopt (client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)))
        {
            log_errno("setsockopt");
            exit(EXIT_FAILURE);
        }

        int rc;
handle_request:
        req->fd = client_fd;

        // where the magic happens
        rc = handle_request(req);

        if (rc == 0 && req->after_task) {
            if (req->after_task(req)) {
                log_errno("request task function failed");
            }
        }

        // NOTE(jason): due to no timeouts taking up connections and generally not that useful
        bool keep_alive = rc != -1 && req->keep_alive;
        //debug_s64(keep_alive);

        // NOTE(jason): overwrite the request data so it can't be used
        // across requests to different clients.
        reuse_request(req);

        // write log after request is finished
        // possibly pass a request trace-id when writing
        flush_log();

        if (keep_alive) {
            goto handle_request;
        } else {
            close(client_fd);
            req->fd = -1;
        }
    }

    return pid;
}

int
upgrade_db_web(const blob_t * db_file)
{
    version_sql_t versions[] = {
        { 1, "create table session_web (session_id integer primary key, user_id integer, expires integer not null, cookie text, redirect_url text, created integer not null default (unixepoch()), modified integer not null default (unixepoch())) strict" }
        , { 2, "create table file_web (file_id integer primary key autoincrement, path text not null unique, size integer not null, content_type integer not null default 1, md5 blob, user_id integer not null, created integer not null default (unixepoch()), modified integer not null default (unixepoch()), status integer not null default 0) strict" }
        , { 3, "create table access_log_web (access_log_id integer primary key autoincrement, created integer not null default (unixepoch()), request_method integer not null , request_path text not null, http_status integer not null, session_id integer not null, user_id integer not null, received integer not null, elapsed_ns integer not null, request_content_length integer not null, response_content_length integer not null)" }
//        , { 4, "alter table file_web add column s3_url text null" }
    };

    return upgrade_db(db_file, versions);
}

// NOTE(jason): prod uses https and no port, dev_mode uses http and port
int
url_dev_web(blob_t * url, const blob_t * hostname, const s64 port, const blob_t * path)
{
    if (dev_mode()) {
        add_blob(url, B("http://"));
        add_blob(url, hostname);
        if (port > 0) {
            write_blob(url, ":", 1);
            add_s64_blob(url, port);
        }
    }
    else {
        add_blob(url, B("https://"));
        add_blob(url, hostname);
    }

    if (valid_blob(path)) {
        if (path->data[0] != '/') {
            write_blob(url, "/", 1);
        }
        add_blob(url, path);
    }

    return 0;
}

int
init_web(const blob_t * res_dir, const blob_t * upload_dir, const blob_t * ffmpeg_path, const s3_t * s3)
{
    res_dir_web = res_dir;

    upload_dir_web = upload_dir;

    // TODO(jason): if s3 is NULL, use local implementation
    if (!s3) {
        error_log("missing s3 config", "web", 1);
        return -1;
    }

    s3_web = s3;

    blob_t * csp = blob(255);
    add_blob(csp, B("default-src 'self' blob: "));
    add_blob(csp, s3->host);
    content_security_policy_web = csp;

    ffmpeg_path_web = ffmpeg_path ? ffmpeg_path : const_blob("ffmpeg");

    if (ensure_dir_file_fu(upload_dir_web, S_IRWXU)) {
        log_errno("make upload dir");
        exit(EXIT_FAILURE);
    }

    init_res_web();

    init_method();
    init_content_type();
    init_http_status();

    init_endpoints();

    return upgrade_db_web(app.main_db_file);
}

// TODO(jason): calculate the max workers based on the amount of RAM in prod mode.
// use a small fixed amount in dev_mode.  if using some amount of memory may
// have to just create new workers until that amount and then save the number
// of workers.  Can't really know the exact amount of memory a worker will use
// some it could require monitoring the amount of available memory and
// adding/removing workers as necessary to keep below max memory usage.
#define MAX_WORKERS_WEB 100
#define DEV_WORKERS_WEB 10

// doesn't return.  calls exit() on failure
// TODO(jason): is param of init_func(config_web_t) pointer a better option for
// doing config?
// load settings directly from app main db?
int
main_web(config_web_t * config)
{
    assert_not_null(config->res_dir);

    if (valid_blob(config->hostname) && empty_blob(config->base_url)) {
        url_dev_web(config->base_url, config->hostname, config->port, NULL);

        //debug_blob(config->base_url);
    }

    // no idea what this should be
    const int backlog = 20;

    if (config->n_dev_workers < 1) config->n_dev_workers = DEV_WORKERS_WEB;
    if (config->n_workers < 1) config->n_workers = MAX_WORKERS_WEB;

    const int n_workers = dev_mode() ? config->n_dev_workers : config->n_workers;

    if (init_web(config->res_dir, config->upload_dir, config->ffmpeg_path, config->s3)) {
        exit(EXIT_FAILURE);
    }

    // setup server socket to listen for requests
    int srv_fd = -1;
    int result;

    struct addrinfo hints = {};
    //memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    // AI_V4MAPPED will make IPv4 addresses look like IPv6 so we'll treat
    // everything as IPv6 internally
    hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;

    blob_t * port = stk_blob(16);
    add_s64_blob(port, config->port);

    struct addrinfo *srv_info;
    result = getaddrinfo(NULL, cstr_blob(port), &hints, &srv_info);
    if (result != 0) {
        error_log(gai_strerror(result), "getaddrinfo", result);
        //errorf("getaddrinfo: %s", gai_strerror(result));
        exit(EXIT_FAILURE);
    }

    // it doesn't seem like there's really a point to looping
    struct addrinfo *p = NULL;
    for (p = srv_info; p != NULL; p = p->ai_next)
    {
        srv_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (srv_fd == -1) {
            log_errno("socket");
            continue;
        }

        int reuseaddr = 1;
        struct timeval timeout;
        // NOTE(jason): setting tv_sec to 10 causes a bunch of accept errors.
        // maybe something to look into later.
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        if (setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int))
                || setsockopt (srv_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))
                || setsockopt (srv_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)))
        {
            log_errno("setsockopt");
            exit(EXIT_FAILURE);
        }

        result = bind(srv_fd, p->ai_addr, p->ai_addrlen);
        if (result == -1) {
            log_errno("bind");
            close(srv_fd);
            continue;
        }

        break;
    }

    freeaddrinfo(srv_info);

    if (p == NULL)
    {
        error_log("failed to bind to an address", "addrinfo", 1);
        exit(EXIT_FAILURE);
    }

    result = listen(srv_fd, backlog);
    if (result == -1) {
        log_errno("listen");
        exit(EXIT_FAILURE);
    }

    pid_t pids[MAX_WORKERS_WEB] = {};

    // XXX(jason): for unknown reason these lines don't get printed.  if
    // they're below the flush_logs() call the print out in every worker????
    //log_var_u64(enum_res_web(wrap_blob("service unavailable"), 0));
    //log_var_blob(res_web.service_unavailable);

    for (int i = 0; i < n_workers; i++)
    {
        pid_t pid = fork_worker_web(srv_fd, config->after_fork_f);
        if (pid > 0) {
            pids[i] = pid;
        }
        else if (pid == -1) {
            // this should kill all workers before exiting?
            // atexit callback
            log_errno("fork_worker_web");
            exit(EXIT_FAILURE);
        }
    }

    flush_log();

    pid_t wpid;
    int wstatus;
    while (true) {
        wpid = waitpid(0, &wstatus, 0);
        if (wpid == -1) {
            log_errno("waitpid");
            continue;
        }

        if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
            debug("worker exited");

            if (WIFEXITED(wstatus)) {
                debugf("exit status: %d", WEXITSTATUS(wstatus));
            }

            if (WIFSIGNALED(wstatus)) {
                debugf("exit signaled %d", WTERMSIG(wstatus));
            }

            for (int i = 0; i < n_workers; i++) {
                if (pids[i] == wpid) {
                    debugf("found pid index: %d", i);

                    pid_t cpid = fork_worker_web(srv_fd, config->after_fork_f);
                    if (cpid == -1) {
                        // TODO(jason): eventually, there will be no more
                        // workers if this keeps failing to replace.
                        // not sure what can be done if that's failing
                        log_errno("fork_worker_web failed to replace worker");
                    }
                    else {
                        debugf("replaced %d with new worker %d", wpid, cpid);
                    }
                    pids[i] = cpid;

                    // TODO(jason): potentially iterate and replace any missing
                    // workers?
                }
            }
        }
    }

    flush_log();

    return 0;
}

int
save_request(request_t * req, const blob_t * path)
{
    if (require_request_body_web(req)) return -1;

    blob_t * header_path = stk_blob(256);
    add_blob(header_path, path);
    add_blob(header_path, B(".header"));
    save_file(header_path, req->request_head);

    return write_prefix_file_fu(path, req->request_body, req->fd, req->request_content_length);
}

int
read_request_body(request_t * req, blob_t * body)
{
    if (req->request_content_length == 0) return 0;

    add_blob(body, req->request_body);
    // TODO(jason): add read_full_file
    if (read_file_fu(req->fd, body) == -1) return -1;

    return 0;
}

