#pragma once

// TODO(jason): possibly should call this http_server (seems to long), httpd
// (daemon?), or something.  doesn't really matter and "web" is short.
// should possibly combine with http client methods

// TODO(jason): this should probably be a config in the db
#define COOKIE_EXPIRES_WEB 86400

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
    E(text, "text/plain", var) \
    E(html, "text/html", var) \
    E(css, "text/css", var) \
    E(javascript, "text/javascript", var) \
    E(png, "image/png", var) \
    E(jpeg, "image/jpeg", var) \
    E(gif, "image/gif", var) \

ENUM_BLOB(content_type, CONTENT_TYPE)

struct {
    stmt_db_t * session_query;
} stmts_web;

#define local_param(f) (param_t){ .field = f, .value = local_blob(f->max_size) }
#define log_param(p) \
    log_var_field(p->field); \
    log_var_blob(p->value); \

typedef enum {
    NO_STORE_CACHE_CONTROL,
    STATIC_ASSET_CACHE_CONTROL
} cache_control_t;

// NOTE(jason): the reason strings is pointless.  maybe need to store the code
// as a number, but this is only used for generating responses.
#define HTTP_STATUS(var, E) \
    E(unknown, "HTTP/1.1 000 N\r\n", var) \
    E(ok, "HTTP/1.1 200 X\r\n", var) \
    E(not_found, "HTTP/1.1 404 X\r\n", var) \
    E(redirect, "HTTP/1.1 303 X\r\n", var) \
    E(permanent_redirect, "HTTP/1.1 301 X\r\n", var) \
    E(method_not_implemented, "HTTP/1.1 501 X\r\n", var) \
    E(bad_request, "HTTP/1.1 400 X\r\n", var) \
    E(internal_server_error, "HTTP/1.1 500 X\r\n", var) \
    E(service_unavailable, "HTTP/1.1 503 X\r\n", var) \

ENUM_BLOB(http_status, HTTP_STATUS)

typedef struct {
    int fd;

    // sqlite uses s64 for rowid so... in theory could use negative ids to mark
    // for deletion
    s64 session_id;
    blob_t * session_cookie;
    s64 user_id;
    blob_t * user_alias;

    // maybe anonymize/mask the ip (hash?)  mainly not for having the actual ip
    // I would think
    //add client ip

    method_t method;
    blob_t * uri;

    content_type_t request_content_type;
    u64 request_content_length;
    bool keep_alive;

    blob_t * request_head;
    blob_t * request_body;

    // portion of uri after a matching wildcard route
    // for /res/fluffy.png and route "/res/*" it's fluffy.png
    blob_t * path;
    blob_t * query;

    content_type_t content_type;
    cache_control_t cache_control;

    http_status_t status;

    //blob_t * status_line;
    blob_t * head;
    blob_t * body;
} request_t;

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
        p->field = fields[i];
        p->value = blob(fields[i]->max_size);

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
    E(see_other,"see other", var) \
    E(connection,"connection", var) \
    E(connection_close,"close", var) \
    E(connection_keep_alive,"keep-alive", var) \
    E(content_type,"content-type", var) \
    E(content_length,"content-length", var) \
    E(cookie,"cookie", var) \
    E(set_cookie,"set-cookie", var) \
    E(location,"location", var) \
    E(name_session_cookie,"z", var) \
    E(session_cookie_attributes,"; max-age=15552000; HttpOnly", var) \
    E(method_not_implemented,"method not implemented", var) \
    E(bad_request,"bad request", var) \
    E(not_found,"not found", var) \
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
    E(res_path,"/res/", var) \
    E(res_path_suffix,"?v=", var) \

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
    // while there are more characters to scan and no error
    ssize_t offset = 0;
    while ((scan_blob(dest, src, '%', &offset)) != -1) {
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
    return -1;
}

int
urlencode_params(blob_t * out, param_t * params, size_t n_params)
{
    if (n_params == 0) return 0;

    write_blob(out, "?", 1);

    for (size_t i = 0; i < n_params; i++) {
        if (i > 0) write_blob(out, "&", 1);
        add_blob(out, params[i].field->name);
        write_blob(out, "=", 1);
        // urlencode
        add_blob(out, params[i].value);
    }

    return 0;
}

// XXX(jason): not possible to tell if a parameter has an empty value, but
// I'm not sure if it matters.  easy enough to add a value even if it's
// ignored
int
urldecode_params(const blob_t * input, param_t * params, size_t n_params)
{
    assert(input != NULL);

    if (empty_blob(input)) return 0;

    assert(params != NULL);
    assert(n_params > 0);

    blob_t * name = tmp_blob();
    blob_t * urlencoded = tmp_blob();

    int count = 0;

    // XXX(jason): not correct parsing
    ssize_t offset = 0;
    while (scan_blob(urlencoded, input, '=', &offset) != -1) {
        reset_blob(name);
        percent_decode_blob(name, urlencoded);

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
    }
}

void
init_web(const blob_t * res_dir)
{
    res_dir_web = res_dir;

    init_res_web();

    init_method();
    init_content_type();
    init_http_status();

    init_endpoints();
}

// TODO(jason): update this to use the ENUM_BLOB
content_type_t
content_type_for_path(const char * path)
{
    char * dot = rindex(path, '.');
    if (dot) dot++;

    // TODO(jason): since limited content types are supported, just assume
    // based on the first char for now.  will really figure out the types on
    // resource load at startup
    switch (*dot) {
        case 'p':
            return png_content_type;
        case 'c':
            return css_content_type;
        case 'j':
            return javascript_content_type;
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
    req->cache_control = NO_STORE_CACHE_CONTROL;
    req->request_content_type = none_content_type;
    req->request_content_length = 0;
    req->keep_alive = true;

    req->session_id = 0;
    req->user_id = 0;
    erase_blob(req->user_alias);

    req->status = 0;

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
    req->request_body = blob(4*1024);

    req->session_cookie = blob(256);
    // TODO(jason): how big should this actually be?  should be fairly short
    // and limited so page layout can be consistent
    req->user_alias = blob(32);

    req->head = blob(1024);
    req->body = blob(4*1024);

    reuse_request(req);

    return req;
}

void
header(blob_t * t, const blob_t *name, const blob_t *value)
{
    vadd_blob(t, name, res_web.header_sep, value, res_web.crlf);
}

void
u64_header(blob_t * b, const blob_t *name, const u64 value)
{
    vadd_blob(b, name, res_web.header_sep);
    add_u64_blob(b, value);
    add_blob(b, res_web.crlf);
}

int
require_request_head_web(request_t * req)
{
    // NOTE(jason): no headers or headers already processed
    if (empty_blob(req->request_head)) return 0;

    blob_t * headers = req->request_head;

    //log_var_blob(headers);

    blob_t * name = tmp_blob();
    blob_t * value = tmp_blob();

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
            req->request_content_length = u64_blob(value, 0);
        }
        else if (case_equal_blob(res_web.connection, name)) {
            req->keep_alive = equal_blob(value, res_web.connection_keep_alive);
        }
        else if (case_equal_blob(res_web.cookie, name)) {
            blob_t * name_cookie = tmp_blob();
            blob_t * value_cookie = tmp_blob();

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

        reset_blob(name);
        reset_blob(value);
    }

    reset_blob(headers);

    return 0;
}

int
require_request_body_web(request_t * req)
{
    require_request_head_web(req);

    //log_var_blob(req->request_body);

    if (req->request_content_length > req->request_body->size) {
        // XXX untested
        info_log("request content length larger than intial read");
        write_fd_blob(req->request_body, req->fd);
    }

    return 0;
}

int
query_params(request_t * req, param_t * params, size_t n_params)
{
    log_var_blob(req->query);

    clear_params(params, n_params);

    return urldecode_params(req->query, params, n_params);
}

int
post_params(request_t * req, param_t * params, size_t n_params)
{
    assert(req->method == post_method);

    require_request_body_web(req);

    assert(req->request_content_type == urlencoded_content_type);

    clear_params(params, n_params);

    return urldecode_params(req->request_body, params, n_params);
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
        u64_header(t, res_web.content_length, (u64)req->body->size);
    }

    if (req->cache_control == STATIC_ASSET_CACHE_CONTROL) {
        header(t, res_web.cache_control, res_web.static_asset_cache_control);
    }
    else {
        header(t, res_web.cache_control, res_web.no_store);
    }

    if (!req->keep_alive) {
        header(t, res_web.connection, res_web.connection_close);
    }

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

// TODO(jason): maybe this should be a macro that sets the blob_t * html.
int
html_response(request_t * req)
{
    return ok_response(req, html_content_type);
}

// generate an error response that formats a body with html about the error 
void
error_response(request_t *req, http_status_t status, blob_t * reason)
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
}

int
internal_server_error_response(request_t * req)
{
    // TODO(jason): should probably have the status code and message as a
    // struct.  maybe a typedef of a general integer/message struct
    error_response(req, internal_server_error_http_status, res_web.internal_server_error);

    return 0;
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
    error_response(req, method_not_implemented_http_status, res_web.method_not_implemented);

    return 0;
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
    error_response(req, bad_request_http_status, res_web.bad_request);

    return 0;
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
    error_response(req, not_found_http_status, res_web.not_found);

    return 0;
}

int
not_found_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);
    return not_found_response(req);
}

int
redirect_params_endpoint(request_t * req, endpoint_t * ep, param_t * params, size_t n_params)
{
    blob_t * url = tmp_blob();
    add_blob(url, ep->path);
    urlencode_params(url, params, n_params);

    return redirect_web(req, url);
}

// NOTE(jason): this is probably fine in most cases as long as it's ok for dest
// to have extra params and it doesn't matter if they're in the url
int
redirect_endpoint(request_t * req, endpoint_t * dest, endpoint_t * src)
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
            return ep->handler(ep, req);
        }
    }

    return 0;
}

param_t *
param_endpoint(endpoint_t * ep, field_id_t field_id)
{
    for (int i = 0; i < ep->n_params; i++) {
        param_t * p = &ep->params[i];
        if (p->field->id == field_id) {
            return p;
        }
    }

    return NULL;
}

// TODO(jason): re-evaluate if this flow is correct.  possibly make all sqlite errors an assert/abort?
int
require_session_web(request_t * req)
{
    if (req->session_id) return 0;

    require_request_head_web(req);

    log_var_blob(req->session_cookie);

    if (valid_blob(req->session_cookie)) {
        sqlite3_stmt * stmt;
        if (prepare_db(db, &stmt, wrap_blob("select session_id, s.user_id, u.alias from session s left join user u on u.user_id = s.user_id where s.cookie = ? and s.expires > unixepoch()")) == 0) {
            text_bind_db(stmt, 1, req->session_cookie);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                req->session_id = s64_db(stmt, 0);
                req->user_id = s64_db(stmt, 1);
                if (req->user_id) blob_db(stmt, 2, req->user_alias);

                log_var_s64(req->session_id);
                log_var_s64(req->user_id);
                log_var_blob(req->user_alias);
            }
        }

        sqlite3_finalize(stmt);

        // update last access time?  I don't think it matters and there should
        // just be a max time session expiration

        if (req->session_id) return 0;
    }

    s64 new_id = random_s64_fu();
    if (!new_id) {
        log_errno("random_s64_fu");
        return -1;
    }

    blob_t * cookie = local_blob(32);
    add_random_blob(cookie, 16);

    log_var_blob(cookie);
    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, wrap_blob("insert into session (session_id, created, modified, expires, cookie) values (?, unixepoch(), unixepoch(), unixepoch() + ?, ?)")) == 0) {
        sqlite3_bind_int64(stmt, 1, new_id);
        sqlite3_bind_int64(stmt, 2, COOKIE_EXPIRES_WEB);
        text_bind_db(stmt, 3, cookie);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            req->session_id = new_id;

            blob_t * c = tmp_blob();

            add_blob(c, res_web.name_session_cookie);
            write_blob(c, "=", 1);
            add_blob(c, cookie);
            //write_hex_blob(c, &new_id, sizeof(new_id));
            add_blob(c, res_web.session_cookie_attributes);
            header(req->head, res_web.set_cookie, c);
        }

        sqlite3_finalize(stmt);

        return 0;
    }

    return -1;
}

int
require_user_web(request_t * req, endpoint_t * auth)
{
    if (require_session_web(req)) return -1;

    log_var_s64(req->user_id);

    if (req->user_id) return 0;

    // TODO(jason): is doing assert here ok since it probably means things are
    // just fucked if this fails or during dev and the sql is wrong
    // alternatively, have a standard fail macro that returns a 500 generic
    // error (fail whale)
    sqlite3_stmt * stmt;
    assert_prepare_db(db, &stmt, "update session set redirect_url = ?, modified = unixepoch() where session_id = ?");
    text_bind_db(stmt, 1, req->uri);
    s64_bind_db(stmt, 2, req->session_id);
    sqlite3_step(stmt);

    if (sqlite3_finalize(stmt)) {
        internal_server_error_response(req);
    }
    else {
        redirect_endpoint(req, auth, NULL);
    }

    return 1;
}

/*
time_t start_time_s;

void
res_url(blob_t * b, blob_t * name)
{
    vadd_blob(b, res_web.res_path, name, res_web.res_path_suffix);
    add_s64_blob(b, (s64)start_time_s);
}
*/

// return a file from the AppDir/res directory with cache-control header set
// for static assets.  the url should include some sort of cache breaker query
// parameter.  there will be a function for generating res urls.
int
res_handler(endpoint_t * ep, request_t * req)
{
    UNUSED(ep);

    blob_t * file_path = local_blob(1024);
    vadd_blob(file_path, res_dir_web, req->path);

    char * path = cstr_blob(file_path);

    log_var_cstr(path);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        log_errno("open path");
        return -1;
    }

    // TODO(jason): read the full file.  all resources should be loaded
    // into memory
    const size_t max_data_size = 65536;
    u8 data[max_data_size];
    ssize_t size = read(fd, data, max_data_size);
    if (size != -1) {
        ok_response(req, content_type_for_path(path));
        req->cache_control = STATIC_ASSET_CACHE_CONTROL;

        write_blob(req->body, data, size);
    }

    return 0;
}

int
handle_request(request_t *req)
{
    ssize_t result = 0;
    const size_t n_iov = 3;
    struct iovec iov[n_iov];

    // TODO(jason): adjust this size so the entire request is read one shot in
    // non-file upload cases.
    blob_t * input = local_blob(16384);
    blob_t * tmp = tmp_blob();

    // NOTE(jason): read in the request line, headers, part of body.  any body
    // will be copied to req->request_body and handlers can read the rest of
    // the body if necessary.
    ssize_t n = write_fd_blob(input, req->fd);
    if (n == -1) {
        log_errno("read request head");
        return -1;
    } else if (n == 0) {
        info_log("empty request");
        return -1;
    }

    struct timespec start;
    // NOTE(jason): have to get the start time for a request after the request
    // line has been read.  Also, chrome will reconnect and not send a request
    // if the connection is closed without a "connection: close" header.  would
    // be required for supporting multiple requests on a connection too.
    timestamp_log(&start);

    //log_blob(input, "input");

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

    log_blob(req->uri, "request uri");
    //log_text(req->path, "request path");
    //log_text(req->query, "request query");

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
    if (remaining_blob(input, offset) > 0) sub_blob(req->request_body, input, offset, -1);

    //log_blob(req->request_head, "head");
    //log_blob(req->request_body, "body");

    // NOTE(jason): don't parse headers here or otherwise process the body.
    // That way no time is wasted parsing headers, etc for a url that doesn't
    // even exist

    route_endpoint(req);

respond:
    // no response default to 404
    if (req->status == 0) {
        not_found_response(req);
    }

    // TODO(jason): handler should've set the status line by this point
    add_response_headers(req);

    const blob_t * status = blob_http_status(req->status);

    // write headers and body that should be in request_t
    // TODO(jason): maybe this should be done in the caller?
    iov[0].iov_base = status->data;
    iov[0].iov_len = status->size;
    iov[1].iov_base = req->head->data;
    iov[1].iov_len = req->head->size;
    iov[2].iov_base = req->body->data;
    iov[2].iov_len = req->body->size;
    result = writev(req->fd, iov, n_iov);

    if (result == -1) log_errno("write response");

    elapsed_log(start, "request-time");

    return 0;
}

#define MAX_CHILDREN_WEB 100

// doesn't return.  calls exit() on failure
int
main_web(const char * srv_port, const blob_t * res_dir, int n_children, int (* after_fork_func)())
{
    assert(res_dir != NULL);
    assert(n_children < MAX_CHILDREN_WEB);

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
        result = setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
        if (result == -1) {
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

    // clear the logs before forking
    flush_log();

    for (int i = 0; i < n_children; i++)
    {
        pid_t pid = fork();

        if (pid == -1) {
            log_errno("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            // parent
            pids[i] = pid;
        }
        else
        {
            // child
            struct sockaddr_in6 client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            // ??? signal(SIGINT, SIG_IGN);

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

                int result;
handle_request:
                req->fd = client_fd;

                // where the magic happens
                result = handle_request(req);

                bool keep_alive = result != -1 && req->keep_alive;

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
        }
    }

    UNUSED(pids);

    flush_log();

    for (;;) pause();

    return 0;
}

