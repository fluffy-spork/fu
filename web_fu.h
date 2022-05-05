#pragma once

// TODO(jason): possibly should call this http_server (seems to long), httpd
// (daemon?), or something.  doesn't really matter and "web" is short.
// should possibly combine with http client methods

typedef struct {
    field_t * field;
    //blob_t * name;
    blob_t * value;
    //blob_t * label;
} param_t;

struct {
    blob_t * get_method;
    blob_t * post_method;
    blob_t * head_method;

    blob_t * space;
    blob_t * crlf;
    blob_t * crlfcrlf;
    blob_t * backslash;
    blob_t * header_sep;

    blob_t * http_11;
    blob_t * ok;
    blob_t * ok_health;
    blob_t * service_unavailable;
    blob_t * see_other;

    blob_t * connection;
    blob_t * connection_close;
    blob_t * connection_keep_alive;
    blob_t * content_type;
    blob_t * content_length;
    blob_t * location;

    blob_t * method_not_implemented;
    blob_t * bad_request;
    blob_t * not_found;

    blob_t * application_octet_stream;
    blob_t * application_x_www_form_urlencoded;
    blob_t * text_plain;
    blob_t * text_html;
    blob_t * text_css;
    blob_t * text_javascript;
    blob_t * image_png;
    blob_t * image_gif;

    blob_t * cache_control;
    blob_t * no_store;
    blob_t * static_asset_cache_control;

    blob_t * res_path;
    blob_t * res_path_suffix;
} res_web;

// where to load resources
const blob_t * res_dir_web;

void
init_web(const blob_t * res_dir)
{
    res_dir_web = res_dir;

    res_web.space = const_blob(" ");
    res_web.crlf = const_blob("\r\n");
    res_web.crlfcrlf = const_blob("\r\n\r\n");
    res_web.backslash = const_blob("/");
    res_web.header_sep = const_blob(":");

    res_web.http_11 = const_blob("HTTP/1.1 ");
    // status 200
    res_web.ok = const_blob("ok");
    res_web.ok_health = const_blob("ok health");
    res_web.service_unavailable = const_blob("service unavailable");
    // status 303
    res_web.see_other = const_blob("see other");

    res_web.get_method = const_blob("GET");
    res_web.post_method = const_blob("POST");
    res_web.head_method = const_blob("HEAD");

    res_web.connection = const_blob("connection");
    res_web.connection_close = const_blob("close");
    res_web.connection_keep_alive = const_blob("keep-alive");
    res_web.content_type = const_blob("content-type");
    res_web.content_length = const_blob("content-length");
    res_web.location = const_blob("location");

    res_web.method_not_implemented = const_blob("method not implemented");
    res_web.bad_request = const_blob("bad request");
    res_web.not_found = const_blob("not found");

    res_web.application_octet_stream = const_blob("application/octet-stream");
    res_web.application_x_www_form_urlencoded = const_blob("application/x-www-form-urlencoded");
    res_web.text_plain = const_blob("text/plain");
    res_web.text_html = const_blob("text/html");
    res_web.text_css = const_blob("text/css");
    res_web.text_javascript = const_blob("text/javascript");
    res_web.image_png = const_blob("image/png");
    res_web.image_gif = const_blob("image/gif");

    res_web.cache_control = const_blob("cache-control");
    res_web.no_store = const_blob("no-store");
    res_web.static_asset_cache_control = const_blob("public, max-age=2592000, immutable");

    res_web.res_path = const_blob("/res/");
    res_web.res_path_suffix = const_blob("?v=");
}

typedef enum {
    NONE_METHOD,
    GET_METHOD,
    HEAD_METHOD,
    POST_METHOD
} method_t;

method_t
parse_method(const blob_t * b)
{
    if (equal_blob(b, res_web.get_method)) {
        return GET_METHOD;
    }
    else if (equal_blob(b, res_web.post_method)) {
        return POST_METHOD;
    }
    else if (equal_blob(b, res_web.head_method)) {
        return HEAD_METHOD;
    }
    else {
        return NONE_METHOD;
    }
}

blob_t *
blob_method(const method_t method)
{
    switch(method) {
        case GET_METHOD:
            return res_web.get_method;
        case POST_METHOD:
            return res_web.post_method;
        case HEAD_METHOD:
            return res_web.head_method;
        default:
            assert(!"invalid method");
    };
}

typedef enum {
    NONE_TYPE,
    BINARY_TYPE,
    TEXT_TYPE,
    HTML_TYPE,
    CSS_TYPE,
    JAVASCRIPT_TYPE,
    PNG_TYPE,
    GIF_TYPE,
    URLENCODED_TYPE
} content_type_t;

typedef enum {
    NO_STORE_CACHE_CONTROL,
    STATIC_ASSET_CACHE_CONTROL
} cache_control_t;

content_type_t
parse_content_type(blob_t * b)
{
    if (equal_blob(b, res_web.application_x_www_form_urlencoded)) {
        return URLENCODED_TYPE;
    }
    // XXX: add the rest or make a general thing for storing id -> blobs
    else if (equal_blob(b, res_web.application_octet_stream)) {
        return BINARY_TYPE;
    }
    else {
        return NONE_TYPE;
    }
}

blob_t *
blob_content_type(content_type_t type)
{
    switch (type) {
        case BINARY_TYPE:
            return res_web.application_octet_stream;
        case TEXT_TYPE:
            return res_web.text_plain;
        case HTML_TYPE:
            return res_web.text_html;
        case CSS_TYPE:
            return res_web.text_css;
        case JAVASCRIPT_TYPE:
            return res_web.text_javascript;
        case PNG_TYPE:
            return res_web.image_png;
        case GIF_TYPE:
            return res_web.image_gif;
        case URLENCODED_TYPE:
            return res_web.application_x_www_form_urlencoded;
        default:
            assert(!"invalid content type");
            return NULL;
    };
}

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
            return PNG_TYPE;
        case 'c':
            return CSS_TYPE;
        case 'j':
            return JAVASCRIPT_TYPE;
        case 'g':
            return GIF_TYPE;
        case 'h':
            return HTML_TYPE;
        case 't':
            return TEXT_TYPE;
        default:
            return BINARY_TYPE;
    }
}

typedef struct {
    int fd;

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

    // response headers probably not necessary to be directly written and
    // should be generated from specific fields.  still need to maintain head
    // buffer over requests to avoid allocation.
    blob_t * head;
    blob_t * body;
} request_t;

// do whatever's necessary to erase/clean a request to be reused for a
// different request.
// would really prefer the whole request to be memset(0) on a single memory
// block.
void
reuse_request(request_t * req)
{
    //req->fd = -1;
    req->method = NONE_METHOD;
    req->content_type = NONE_TYPE;
    req->request_content_type = NONE_TYPE;
    req->request_content_length = 0;
    req->keep_alive = true;

    erase_blob(req->uri);
    erase_blob(req->path);
    erase_blob(req->query);
    erase_blob(req->head);
    erase_blob(req->body);
    erase_blob(req->request_head);
    erase_blob(req->request_body);
}

request_t *
new_request()
{
    request_t * req = malloc(sizeof(request_t));
    if (!req) return NULL;

    // decide if these sizes matter
    req->uri = medium_blob();
    req->path = medium_blob();
    req->query = medium_blob();

    req->request_head = blob(16*1024);
    req->request_body = blob(1024*1024);

    req->head = medium_blob();
    req->body = blob(1024*1024);

    reuse_request(req);

    return req;
}

int
require_request_head(request_t * req)
{
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
            req->request_content_type = parse_content_type(value);
        }
        else if (case_equal_blob(res_web.content_length, name)) {
            req->request_content_length = u64_blob(value, 0);
        }
        else if (case_equal_blob(res_web.connection, name)) {
            req->keep_alive = equal_blob(value, res_web.connection_keep_alive);
        }

        //log_var_blob(name);
        //log_var_blob(value);

        reset_blob(name);
        reset_blob(value);
    }

    return 0;
}

int
require_request_body(request_t * req)
{
    require_request_head(req);

    //log_var_blob(req->request_body);

    if (req->request_content_length > req->request_body->size) {
        // XXX untested
        info_log("request content length larger than intial read");
        write_fd_blob(req->request_body, req->fd);
    }

    return 0;
}

// TODO(jason): the code could probably be 3 bytes as ascii numbers instead of
// an int.  It would be much smaller and checking for 1, 2, 3, 4, or 5 level
// errors would only be a single check on the first digit.  these are mostly
// just going to be converted to text in the response anyway.
// code and reason will probably be combined into constants anyway.
void
status_line(blob_t * b, s64 code, const blob_t * reason)
{
    add_blob(b, res_web.http_11);
    add_s64_blob(b, code);
    vadd_blob(b, res_web.space, reason, res_web.crlf);
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

void
response_headers(blob_t *t, request_t *req)
{
    if (req->content_type != NONE_TYPE) {
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
    status_line(req->head, 200, res_web.ok);

    return 0;
}

int
html_response(request_t * req)
{
    return ok_response(req, HTML_TYPE);
}

// generate an error response that formats a body with html about the error 
void
error_response(request_t *req, int code, const blob_t * reason)
{
    req->content_type = HTML_TYPE;
    status_line(req->head, code, reason);

    page_begin(req->body, reason);
    h1(req->body, reason);
    page_end(req->body);
}

int
redirect_response(request_t * req, const blob_t * url)
{
    //req->content_type = NONE_TYPE;

    // TODO(jason): 303 may not actually work.  it's technically correct by the standard,
    // but 302 has always been used for redirecting with a GET after a POST
    status_line(req->head, 303, res_web.see_other);
    // TODO(jason): this likely needs to be an absolute url.  relative works
    // fine in chrome, but curl doesn't seem like it's working correctly.  May
    // not matter
    header(req->head, res_web.location, url);

    return 0;
}

int
method_not_implemented(request_t * req)
{
    // TODO(jason): should probably have the status code and message as a
    // struct.  maybe a typedef of a general integer/message struct
    error_response(req, 501, res_web.method_not_implemented);

    return 0;
}

int
bad_request(request_t * req)
{
    error_response(req, 400, res_web.bad_request);

    return 0;
}

int
not_found(request_t * req)
{
    error_response(req, 404, res_web.not_found);

    return 0;
}

bool
hex_char(u8 c)
{
    return (c >= '0' && c <= '9') || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
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

// XXX(jason): not possible to tell if a parameter has an empty value, but
// I'm not sure if it matters.  easy enough to add a value even if it's
// ignored
int
urlencoded_params(const blob_t * input, param_t * params, size_t n_params)
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

int
query_params(request_t * req, param_t * params, size_t n_params)
{
    log_var_blob(req->query);

    return urlencoded_params(req->query, params, n_params);
}

int
post_params(request_t * req, param_t * params, size_t n_params)
{
    assert(req->method == POST_METHOD);

    require_request_body(req);

    assert(req->request_content_type == URLENCODED_TYPE);

    return urlencoded_params(req->request_body, params, n_params);
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
res_handler(request_t * req)
{
    blob_t * file_path = local_blob(1024);
    vadd_blob(file_path, res_dir_web, req->path);

    char path[256];
    read_blob(file_path, path, 256);

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

typedef int (* handler_func)(request_t *);

typedef struct {
    blob_t * path;
    // TODO(jason): I'm pretty sure in practice this isn't necessary and all
    // routes should just be "begins with" and they're searched in the order added
    bool wildcard;
    method_t method;
    handler_func func;
} request_handler_t;

typedef struct {
    request_handler_t handlers[256];
    size_t size;
} router_t;

static router_t router_web;

// TODO(jason): should multiple methods be allowed on a path?
// /users/new - get form
// /users/add - post the new user form data
// /users/delete?id=
// /users/edit?id= - get form
// /users/update - post edit form data
int
add_route(const char * path, method_t method, handler_func func)
{
    request_handler_t * h = &router_web.handlers[router_web.size];

    blob_t * b = const_blob(path);

    // XXX: alloc
    h->wildcard = ends_with_char_blob(b, '*');
    if (h->wildcard) b->size--;

    h->path = b;
    h->method = method;
    h->func = func;

    router_web.size++;

    return 0;
}

bool
match_route(request_handler_t * handler, request_t * req)
{
    if (req->method != handler->method) return false;

    if (handler->wildcard) {
        return begins_with_blob(req->path, handler->path);
    } else {
        return equal_blob(req->path, handler->path);
    }
}

int
route(request_t * req)
{
    request_handler_t * h = NULL;

    // use request uri to find handler
    for (size_t i = 0; i < router_web.size; i++) {
        if (match_route(&router_web.handlers[i], req)) {
            h = &router_web.handlers[i];
            //log_blob(h->path, "found handler");
        }
    }

    if (h) h->func(req);

    return 0;
}

int
handle_request(request_t *req)
{
    ssize_t result = 0;
    struct iovec iov[2];

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
        bad_request(req);

        goto respond;
    }

    req->method = parse_method(tmp);
    if (req->method == NONE_METHOD) {
        method_not_implemented(req);

        goto respond;
    }

    reset_blob(tmp);

    // parse uri

    if (scan_blob(req->uri, input, ' ', &offset) == -1) {
        bad_request(req);

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

    if (scan_blob(tmp, input, '\n', &offset) == -1) {
        bad_request(req);

        goto respond;
    }

    //log_blob(tmp, "http version");

    // copy the headers from input to req->request_headers(?) and req->request_body

    if (scan_blob_blob(req->request_head, input, res_web.crlfcrlf, &offset) == -1) {
        bad_request(req);

        goto respond;
    }

    // NOTE(jason): ignore for requests with an empty body
    if (remaining_blob(input, offset) > 0) sub_blob(req->request_body, input, offset, -1);

    //log_blob(req->request_head, "head");
    //log_blob(req->request_body, "body");

    // NOTE(jason): don't parse headers here or otherwise process the body.
    // That way no time is wasted parsing headers, etc for a url that doesn't
    // even exist

    route(req);

respond:
    // no response default to 404
    if (empty_blob(req->head)) {
        not_found(req);
    }

    response_headers(req->head, req);

    // write headers and body that should be in request_t
    // TODO(jason): maybe this should be done in the caller?
    iov[0].iov_base = req->head->data;
    iov[0].iov_len = req->head->size;
    iov[1].iov_base = req->body->data;
    iov[1].iov_len = req->body->size;
    result = writev(req->fd, iov, 2);

    if (result == -1) log_errno("write response");

    elapsed_log(start, "request-time");

    return 0;
}

#define MAX_CHILDREN_WEB 100

// doesn't return.  calls exit() on failure
int
main_web(const char * srv_port, const blob_t * res_dir, int n_children)
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

    // clear the logs before forking
    flush_log();

    for (int i = 0; i < n_children; i++)
    {
        pid_t pid = fork();
        assert(pid >= 0);

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

            while (1)
            {
                int client_fd = accept(srv_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_fd == -1) {
                    log_errno("accept");
                    continue;
                }

                req->fd = client_fd;

                int result = 0;

handle_request:
                // where the magic happens
                result = handle_request(req);

                // this keep alive stuff doesn't work
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

    (void)pids;

    flush_log();

    for (;;) pause();

    return 0;
}

