#pragma once

#define MAX_REQUEST_BODY 4096
#define MAX_RESPONSE_BODY 8192

#define MAX_SIZE_FILE_UPLOAD 100*1024*1024

// TODO(jason): possibly should call this http_server (seems to long), httpd
// (daemon?), or something.  doesn't really matter and "web" is short.
// should possibly combine with http client methods

// TODO(jason): this should probably be a config in the db
#define COOKIE_EXPIRES_WEB (86400 * 365)

#define METHOD(var, E) \
    E(none, "NONE", var) \
    E(get, "GET", var) \
    E(post, "POST", var) \
    E(head, "HEAD", var) \

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

ENUM_BLOB(content_type, CONTENT_TYPE)

typedef enum {
    NO_STORE_CACHE_CONTROL,
    STATIC_ASSET_CACHE_CONTROL,
    USER_FILE_CACHE_CONTROL
} cache_control_t;

// NOTE(jason): the reason strings is pointless.  maybe need to store the code
// as a number, but this is only used for generating responses.
// how can  payload too large max size be configurable?
#define HTTP_STATUS(var, E) \
    E(unknown, "HTTP/1.1 000 N\r\n", var) \
    E(ok, "HTTP/1.1 200 X\r\n", var) \
    E(created, "HTTP/1.1 201 X\r\n", var) \
    E(not_found, "HTTP/1.1 404 X\r\n", var) \
    E(redirect, "HTTP/1.1 303 X\r\n", var) \
    E(permanent_redirect, "HTTP/1.1 301 X\r\n", var) \
    E(method_not_implemented, "HTTP/1.1 501 X\r\n", var) \
    E(bad_request, "HTTP/1.1 400 bad request\r\n", var) \
    E(internal_server_error, "HTTP/1.1 500 X\r\n", var) \
    E(service_unavailable, "HTTP/1.1 503 X\r\n", var) \
    E(payload_too_large, "HTTP/1.1 413 file too large: max size 100MB\r\n", var) \
    E(expect_continue, "100 X\r\n", var) \
    E(payment_required, "HTTP/1.1 402 X\r\n", var) \

ENUM_BLOB(http_status, HTTP_STATUS)

typedef struct request_s request_t;
typedef int (* request_task_f)(request_t *);

struct request_s {
    int fd;

    // sqlite uses s64 for rowid so... in theory could use negative ids to mark
    // for deletion
    s64 session_id;
    blob_t * session_cookie;
    s64 user_id;
    blob_t * user_name;

    // maybe anonymize/mask the ip (hash?)  mainly not for having the actual ip
    // I would think
    //add client ip

    method_t method;
    blob_t * uri;

    content_type_t request_content_type;
    size_t request_content_length;
    bool keep_alive;
    bool expect_continue;

    blob_t * request_head;
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
};

typedef struct endpoint_s endpoint_t;
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

endpoint_t *
endpoint_by_id(endpoint_id_t id)
{
    assert(id < endpoints.n_list);
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

const endpoint_t * login_endpoint_web;

#define RES_WEB(var, E) \
    E(space," ", var) \
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
    E(cookie,"cookie", var) \
    E(set_cookie,"set-cookie", var) \
    E(location,"location", var) \
    E(x_frame_options,"x-frame-options", var) \
    E(deny_x_frame_options,"DENY", var) \
    E(content_security_policy,"content-security-policy", var) \
    E(self_default_content_security_policy,"default-src 'self' blob:", var) \
    E(name_session_cookie,"z", var) \
    E(session_cookie_attributes,"; path=/; max-age=15552000; httponly; samesite=strict", var) \
    E(method_not_implemented,"method not implemented", var) \
    E(bad_request,"bad request", var) \
    E(not_found,"not found", var) \
    E(payment_required, "payment required", var) \
    E(application_octet_stream,"application/octet-stream", var) \
    E(application_x_www_form_urlencoded,"application/x-www-form-urlencoded", var) \
    E(text_plain,"text/plain", var) \
    E(text_html,"text/html", var) \
    E(text_css,"text/css", var) \
    E(text_javascript,"text/javascript", var) \
    E(image_png,"image/png", var) \
    E(image_gif,"image/gif", var) \
    E(cache_control,"cache-control", var) \
    E(no_store,"no-store", var) \
    E(static_asset_cache_control,"public, max-age=2592000, immutable", var) \
    E(user_file_cache_control,"private, max-age=2592000, immutable", var) \
    E(res_path,"/res/", var) \
    E(res_path_suffix,"?v=", var) \
    E(file_table,"file", var) \
    E(poster_kind,"poster", var) \

ENUM_BLOB(res_web, RES_WEB)

u8
hex_char(u8 c)
{
    return ((c >= '0' && c <= '9') || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
        ? c
        : 0;
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

    blob_t * name = local_blob(255);
    blob_t * urlencoded = local_blob(1024);

    int count = 0;

    // XXX(jason): not correct parsing
    ssize_t offset = 0;
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

void
init_web(const blob_t * res_dir)
{
    res_dir_web = res_dir;
    // TODO(jason): improve.  probably move to an app data dir (app user home
    // dir) and then this is a subdir of that
    upload_dir_web = const_blob("uploads/");

    if (mkdir_file_fu(upload_dir_web, S_IRWXU)) {
        if (errno != EEXIST) {
            log_errno("make upload dir");
            exit(EXIT_FAILURE);
        }
    }

    init_res_web();

    init_method();
    init_content_type();
    init_http_status();

    init_endpoints();

    // TODO(jason): make this a config param
    login_endpoint_web = endpoints.email_login_code;
}

content_type_t
content_type_magic(const blob_t * data)
{
    // this could all be way better
    static char jpeg_magic[] = { 0xFF, 0xD8, 0xFF };
    static char png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    static char gif_magic[] = { 0x47, 0x49, 0x46, 0x38 };
    //static char mp4_magic[] = { 0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32 };
    static char mp4_magic[] = { 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6F, 0x6D };
    static char mp42_magic[] = { 0x66, 0x74, 0x79, 0x70, 0x6D, 0x70, 0x34, 0x32 };
    static char mov_magic[] = { 0x66, 0x74, 0x79, 0x70 };

    blob_t * magic = local_blob(40);
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
content_type_for_path(const blob_t * path)
{
    //debug_blob(path);
    char * dot = rindex(cstr_blob(path), '.');
    //char * dot = NULL;
    if (dot) {
        char first_char = *(dot + 1);

        // TODO(jason): since limited content types are supported, just assume
        // based on the first char for now.  will really figure out the types on
        // resource load at startup
        switch (first_char) {
            case 'p':
                return png_content_type;
            case 'c':
                return css_content_type;
            case 'j':
                {
                    char second = *(dot + 2);
                    if (second == 'p') {
                        return jpeg_content_type;
                    } else if (second == 's') {
                        return javascript_content_type;
                    }
                }
                break;
            case 'g':
                return gif_content_type;
            case 'h':
                return html_content_type;
            case 't':
                return text_content_type;
            default:
                return binary_content_type;
        }
    }

    // if there's no extension, attempt to identify based on magic number
    int fd = open_read_file_fu(path);
    if (fd == -1) {
        log_errno("open_read_file_fu");
        return binary_content_type;
    }

    blob_t * magic = local_blob(255);
    if (read_file_fu(fd, magic) == -1) {
        log_errno("read_file_fu");
        return binary_content_type;
    }

    return content_type_magic(magic);
}

bool
video_content_type(content_type_t type)
{
    if (type == mp4_content_type) {
        return true;
    }

    return false;
}

bool
image_content_type(content_type_t type)
{
    switch (type) {
        case jpeg_content_type:
        case png_content_type:
        case gif_content_type:
            return true;
        default:
            return false;
    }
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
    req->content_length = 0;
    req->cache_control = NO_STORE_CACHE_CONTROL;
    req->request_content_type = none_content_type;
    req->request_content_length = 0;
    req->keep_alive = true;
    req->expect_continue = false;

    req->session_id = 0;
    req->user_id = 0;
    erase_blob(req->user_name);

    req->status = 0;

    req->after_task = NULL;
    req->id_task = 0;

    erase_blob(req->uri);
    erase_blob(req->path);
    erase_blob(req->query);
    erase_blob(req->head);
    erase_blob(req->body);
    erase_blob(req->request_head);
    erase_blob(req->request_body);
    erase_blob(req->session_cookie);
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

    req->request_head = blob(1024);
    req->request_body = blob(MAX_REQUEST_BODY);

    req->session_cookie = blob(256);
    // TODO(jason): how big should this actually be?  should be fairly short
    // and limited so page layout can be consistent
    req->user_name = blob(32);

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

// TODO(jason): probably remove and add s64_header
void
u64_header(blob_t * b, const blob_t *name, const u64 value)
{
    vadd_blob(b, name, res_web.header_sep);
    add_u64_blob(b, value);
    add_blob(b, res_web.crlf);
}

// TODO(jason): this probably shouldn't be "require"
int
require_request_head_web(request_t * req)
{
    // NOTE(jason): no headers or headers already processed
    if (empty_blob(req->request_head)) return 0;

    blob_t * headers = req->request_head;
    //debug_blob(headers);

    blob_t * name = local_blob(255);
    blob_t * value = local_blob(1024);

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
            blob_t * name_cookie = local_blob(255);
            blob_t * value_cookie = local_blob(255);

            ssize_t coff = 0;
            while (scan_blob(name_cookie, value, '=', &coff) != -1) {
                if (equal_blob(name_cookie, res_web.name_session_cookie)) {
                    if (scan_blob(value_cookie, value, ';', &coff) != -1) {
                        add_blob(req->session_cookie, value_cookie);
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

    reset_blob(headers);

    return 0;
}

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

    if (empty_blob(req->query)) return 0;

    param_t * params = ep->params;
    size_t n_params = ep->n_params;;

    return urldecode_params(req->query, params, n_params);
}

int
post_params(request_t * req, endpoint_t * ep)
{
    param_t * params = ep->params;
    size_t n_params = ep->n_params;;

    assert(req->method == post_method);

    require_request_head_web(req);

    assert(req->request_content_type == urlencoded_content_type);

    require_request_body_web(req);
    return urldecode_params(req->request_body, params, n_params);
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
    if (req->content_length > 0) {
        u64_header(t, res_web.content_length, req->content_length);
    }
    else {
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
    header(t, res_web.content_security_policy, res_web.self_default_content_security_policy);

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

// generate an error response that formats a body with html about the error 
int
error_response(request_t * req, http_status_t status, blob_t * reason)
{
    // NOTE(jason): have to overwrite existing head and body in error
    // case and not just append to any existing code.
    reset_blob(req->head);
    reset_blob(req->body);

    req->content_type = html_content_type;
    req->status = status;

    blob_t * html = req->body;

    page_begin(reason);
    h1(reason);
    page_end();

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

int
bad_request_response(request_t * req)
{
    return error_response(req, bad_request_http_status, res_web.bad_request);
}

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
    blob_t * file_path = local_blob(255);
    vadd_blob(file_path, dir, path);

    //debug_blob(file_path);

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

    if (content_type == 0) {
        content_type = content_type_for_path(file_path);
    }

    //debug_s64(len);

    // TODO(jason): need to look at the actual data for content type
    ok_response(req, content_type);
    // TODO(jason): not sure if all file responses should be cacheable
    req->cache_control = user_id  ? USER_FILE_CACHE_CONTROL : STATIC_ASSET_CACHE_CONTROL;
    req->content_length = len;
    add_response_headers(req);

    const blob_t * status = blob_http_status(req->status);

    if (write_file_fu(req->fd, status) == -1) {
        internal_server_error_response(req);
        return log_errno("write_file_fu status");
    }

    if (write_file_fu(req->fd, req->head) == -1) {
        internal_server_error_response(req);
        return log_errno("write_file_fu head");
    }

    if (copy_file_fu(req->fd, fd, len) == -1) {
        return log_errno("copy_file_fu");
    }

    return req->content_length;
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
    blob_t * url = local_blob(255);
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

// NOTE(jason): this is probably fine in most cases as long as it's ok for dest
// to have extra params and it doesn't matter if they're in the url
int
redirect_endpoint(request_t * req, const endpoint_t * dest, endpoint_t * src)
{
    return src != NULL
        ? redirect_params_endpoint(req, dest, src->params, src->n_params)
        : redirect_web(req, dest->path);
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
            int result = ep->handler(ep, req);
            // TODO(jason): make sure parameters are cleared so don't get used
            // by a later request.  this doesn't seem very good.  maybe
            // endpoint should be on the request and clear params in
            // reuse_request
            clear_params(ep->params, ep->n_params);
            return result;
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

// Sets the param value if it's empty
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

// TODO(jason): re-evaluate if this flow is correct.  possibly make all sqlite errors an assert/abort?
// this create seems odd i don't think it's quite right.  the only place that
// sessions seem to need to be created is the email_login_code_handler.
// everywhere else is only read.  until maybe there's a shopping cart or
// something.  that could also directly create a session if necessary too.
//
// TODO(jason): is another random value cookie needed that's only sent to the
// login path or something that helps security?  i'm not sure what i'm talking about.
int
require_session_web(request_t * req, bool create)
{
    if (req->session_id) return 0;

    require_request_head_web(req);

    //debug_blob(req->session_cookie);

    if (valid_blob(req->session_cookie)) {
        sqlite3_stmt * stmt;

        if (prepare_db(db, &stmt, B("select session_id, u.user_id, u.name from session s left join user u on u.user_id = s.user_id where s.cookie = ? and s.expires > unixepoch()"))) {
            internal_server_error_response(req);
            return -1;
        }

        text_bind_db(stmt, 1, req->session_cookie);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            req->session_id = s64_db(stmt, 0);
            req->user_id = s64_db(stmt, 1);
            if (req->user_id) blob_db(stmt, 2, req->user_name);

            log_var_s64(req->session_id);
            log_var_s64(req->user_id);
            log_var_blob(req->user_name);
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

        blob_t * cookie = local_blob(32);
        add_random_blob(cookie, 16);

        log_var_blob(cookie);

        sqlite3_stmt * stmt;
        if (prepare_db(db, &stmt, B("insert into session (session_id, created, modified, expires, cookie) values (?, unixepoch(), unixepoch(), unixepoch() + ?, ?)"))) {
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

        blob_t * c = local_blob(255);

        add_blob(c, res_web.name_session_cookie);
        write_blob(c, "=", 1);
        add_blob(c, cookie);
        //write_hex_blob(c, &new_id, sizeof(new_id));
        add_blob(c, res_web.session_cookie_attributes);
        header(req->head, res_web.set_cookie, c);
    }

    return 0;
}

int
end_session_web(request_t * req)
{
    if (!req->session_id) return 0;

    // NOTE(jason): set session id cookie to blank?  probably doesn't matter as
    // it will be invalid the next request anyway.

    blob_t * sql = B("update session set expires = unixepoch(), modified = unixepoch() where session_id = ?1");
    return exec_s64_pi_db(db, sql, NULL, req->session_id);
}

int
require_user_web(request_t * req, const endpoint_t * auth)
{
    if (require_session_web(req, false)) return -1;

    //log_var_s64(req->user_id);

    if (req->user_id) return 0;

    if (!auth) {
        not_found_response(req);
        return -1;
    }

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, B("update session set redirect_url = ?, modified = unixepoch() where session_id = ?"))) {
        internal_server_error_response(req);
        return -1;
    }

    log_var_s64(req->session_id);

    text_bind_db(stmt, 1, req->uri);
    s64_bind_db(stmt, 2, req->session_id);
    sqlite3_step(stmt);
    if (finalize_db(stmt)) {
        internal_server_error_response(req);
    }
    else {
        redirect_endpoint(req, auth, NULL);
    }

    return 1;
}

int
optional_user_web(request_t * req)
{
    return require_session_web(req, false);
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

int
insert_file(db_t * db, s64 * file_id, s64 user_id, blob_t * name, s64 size, s64 content_type)
{
    param_t params[] = {
        local_param(fields.name),
        local_param(fields.size),
        local_param(fields.content_type),
    };

    add_blob(params[0].value, name);
    add_s64_blob(params[1].value, size);
    add_s64_blob(params[2].value, content_type);

    return insert_params(db, res_web.file_table, file_id, user_id, params, array_size_fu(params));
}

int
file_info(db_t * db, param_t * file_id, param_t * name, param_t * content_type)
{
    return by_id_db(db, B("select name, content_type from file where file_id = ?"), s64_blob(file_id->value, 0), name, content_type);
}

#define MAX_CMD_WEB 1024

int
path_file_web(blob_t * path, const blob_t * name)
{
    add_blob(path, upload_dir_web);
    add_blob(path, name);

    return path->error;
}

// TODO(jason): don't like calling an external prog.  eventually replace with header code
int
_ffmpeg_call_web(const blob_t * input, const blob_t * output, const blob_t * filter)
{
    char * fmt_cmd = "ffmpeg -hide_banner -loglevel error -nostdin -i %s %s %s";

    char cmd[MAX_CMD_WEB];
    if (snprintf(cmd, MAX_CMD_WEB, fmt_cmd, cstr_blob(input), cstr_blob(filter), cstr_blob(output)) < 0) {
        log_errno("ffmpeg cmd format failed");
        return -1;
    }

    info_log(cmd);

    int rc = system(cmd);
    if (rc) {
        log_errno("failed to process media file");
        return -1;
    }

    return 0;
}

// TODO(jason): don't like calling an external prog.  eventually replace with header code
int
resize_image_web(const blob_t * input)
{
    blob_t * output = local_blob(255);
    vadd_blob(output, input, B(".jpg"));

    char * fmt_cmd = "convert-im6 -resize 1024x %s %s";

    char cmd[MAX_CMD_WEB];
    if (snprintf(cmd, MAX_CMD_WEB, fmt_cmd, cstr_blob(input), cstr_blob(output)) < 0) {
        log_errno("imagemagick convert cmd format failed");
        return -1;
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
extract_poster_web(const blob_t * input)
{
    blob_t * output = local_blob(255);
    vadd_blob(output, input, B(".jpg"));

    return _ffmpeg_call_web(input, output, B("-y -vframes 1 -vf scale=1024:-1"));
}

int
transcode_video_web(const blob_t * input)
{
    blob_t * output = local_blob(255);
    vadd_blob(output, input, B(".mp4"));

    return _ffmpeg_call_web(input, output, B("-y -vf scale=512:-1 -c:v libx264 -preset ultrafast -crf 18 -an -pix_fmt yuv420p -movflags +faststart"));
}

int
process_media_web(param_t * file_id)
{
    def_param(name);
    def_param(content_type);

    if (file_info(db, file_id, &name, &content_type)) {
        log_error_db(db, "failed to get file info");
        return -1;
    }

    content_type_t type = s64_blob(content_type.value, 0);

    blob_t * input = local_blob(255);
    path_file_web(input, name.value);

    if (video_content_type(type)) {
        // make a poster image and transcode to consistent video format
        debug("XXXXX processing video file XXXX");
        if (extract_poster_web(input)
                || transcode_video_web(input))
        {
            return -1;
        }

        return 0;
    }
    else if (image_content_type(type)) {
        // remove exif and other identifying info
        //debug("XXXXX resizing static image XXXX");
        return resize_image_web(input);
    }

    return 0;
}

int
process_media_task_web(request_t * req)
{
    s64 id = req->id_task;

    if (id < 1) {
        error_log("missing id_task for file_id", "web_fu", 1);
    }

    def_param(file_id);
    add_s64_blob(file_id.value, id);

    return process_media_web(&file_id);
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
        type = mp4_content_type;
        //return bad_request_response(req);
    }


    blob_t * file_key = local_blob(32);
    fill_random_blob(file_key);

    blob_t * path = local_blob(255);
    add_blob(path, upload_dir_web);
    add_blob(path, file_key);

    s64 file_id;
    if (insert_file(db, &file_id, req->user_id, file_key, req->request_content_length, type)) {
        internal_server_error_response(req);
        return -1;
    }

    // TODO(jason): switch to storing the files with a SHA256 or something
    // filename to avoid duplicate uploads.
    // could potentially avoid uploads if the digest header is used with
    // requests or some other method to avoid even uploading dupes.
    if (write_prefix_file_fu(path, req->request_body, req->fd, req->request_content_length) == -1) {
        internal_server_error_response(req);
        return -1;
    }

    if (video_content_type(type) || image_content_type(type)) {
        req->after_task = process_media_task_web;
        req->id_task = file_id;
    }

    blob_t * location = local_blob(255);
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
        debug("empty param");
        internal_server_error_response(req);
        return -1;
    }

    def_param(name);
    def_param(content_type);

    if (file_info(db, file_id, &name, &content_type)) {
        not_found_response(req);
        return -1;
    }

    content_type_t type = s64_blob(content_type.value, 0);

    param_t * kind = param_endpoint(ep, kind_field);
    blob_t * filename = NULL;
    if (image_content_type(type) || equal_blob(kind->value, res_web.poster_kind)) {
        filename = local_blob(name.value->capacity + 4);
        vadd_blob(filename, name.value, B(".jpg"));
        type = jpeg_content_type;
    }
    else if (video_content_type(type)) {
        filename = local_blob(name.value->capacity + 4);
        vadd_blob(filename, name.value, B(".mp4"));
        type = mp4_content_type;
    }
    else {
        filename = name.value;
    }

    //debug_blob(filename);

    if (!filename) {
        return bad_request_response(req);
    }

    int rc = file_response(req, upload_dir_web, filename, type, req->user_id);
    if (rc == -1 && errno == ENOENT) {
        if (process_media_web(file_id)) {
            internal_server_error_response(req);
            return -1;
        }

        return file_response(req, upload_dir_web, filename, type, req->user_id);
    }

    return rc;
}

int
handle_request(request_t *req)
{
    ssize_t result = 0;
    const size_t n_iov = 3;
    struct iovec iov[n_iov];

    // TODO(jason): adjust this size so the entire request is read one shot in
    // non-file upload cases.
    blob_t * input = local_blob(MAX_RESPONSE_BODY/2);
    blob_t * tmp = local_blob(1024);

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
        log_errno("read request head");
        return -1;
    } else if (n == 0) {
        // TODO(Jason): seems like this is happening way more than desirable.
        // although I guess it always has to happened for all requests that do
        // keep alive?  maybe it's just stupid to log.
        info_log("empty request");
        return -1;
    }

    struct timespec start;
    // NOTE(jason): have to get the start time for a request after the request
    // line has been read.  Also, chrome will reconnect and not send a request
    // if the connection is closed without a "connection: close" header.  would
    // be required for supporting multiple requests on a connection too.
    timestamp_log(&start);

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

    log_var_blob(req->uri);
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
        bad_request_response(req);

        goto respond;
    }

    // NOTE(jason): ignore for requests with an empty body
    if (remaining_blob(input, offset) > 0
            && sub_blob(req->request_body, input, offset, -1) == -1) {
        // NOTE(jason): I spent a lot of time on a bug caused by having body
        // smaller.  the real fix is input is a fraction of the
        // MAX_RESPONSE_BODY size
        if (req->request_body->error == ENOSPC) {
            dev_error(req->request_body->capacity >= input->capacity);
            debug("request body somehow smaller than input size");
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
            elapsed_log(start, "request-time");
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

    if (!sqlite3_get_autocommit(db)) {
        dev_error("sqlite transaction open, rolling back");
        // NOTE(jason): the rollback is here just so the db won't be borked
        rollback_db(db);
    }

    elapsed_log(start, "request-time");

    return 0;
}

pid_t
fork_worker_web(int srv_fd, int (* after_fork_func)())
{
    // clear the logs before forking
    flush_log();

    pid_t pid = fork();

    if (pid != 0) {
        return pid;
    }

    // child
    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    request_t * req = new_request();

    if (!req) {
        log_errno("new_request");
        exit(EXIT_FAILURE);
    }

    // NOTE(jason): callback so application can open db connections,
    // etc
    after_fork_func();

    flush_log();

    while (1)
    {
        int client_fd = accept(srv_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            log_errno("accept");
            continue;
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

#define MAX_CHILDREN_WEB 1000

// doesn't return.  calls exit() on failure
int
main_web(const char * srv_port, const blob_t * res_dir, int n_children, int (* after_fork_func)())
{
    assert(res_dir != NULL);
    assert(n_children <= MAX_CHILDREN_WEB);

    // no idea what this should be
    const int backlog = 20;

    init_web(res_dir);

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

    struct addrinfo *srv_info;
    result = getaddrinfo(NULL, srv_port, &hints, &srv_info);
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

    pid_t pids[MAX_CHILDREN_WEB] = {};

    // XXX(jason): for unknown reason these lines don't get printed.  if
    // they're below the flush_logs() call the print out in every child????
    //log_var_u64(enum_res_web(wrap_blob("service unavailable"), 0)); 
    //log_var_blob(res_web.service_unavailable);

    for (int i = 0; i < n_children; i++)
    {
        pid_t pid = fork_worker_web(srv_fd, after_fork_func);
        if (pid > 0) {
            pids[i] = pid;
        }
        else if (pid == -1) {
            // this should kill all children before exiting?
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

            for (int i = 0; i < n_children; i++) {
                if (pids[i] == wpid) {
                    debugf("found pid index: %d", i);

                    pid_t cpid = fork_worker_web(srv_fd, after_fork_func);
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

    return 0;
}

