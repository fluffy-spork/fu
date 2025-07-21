#pragma once

// TODO(jason): add a method for common curl option setting

#include <curl/curl.h>

#define MAX_SIZE_URL 1024

#define METHOD_HTTP(var, E) \
    E(none, "NONE", var) \
    E(get, "GET", var) \
    E(post, "POST", var) \
    E(head, "HEAD", var) \
    E(put, "PUT", var) \

ENUM_BLOB(method_http, METHOD_HTTP)

#define CACHE_CONTROL_HTTP(var, E) \
    E(unknown, "unknown", var) \
    E(no_store, "no-store", var) \
    E(public_immutable, "public,max-age=604800,immutable", var) \
    E(user_immutable, "private,max-age=604800,immutable", var) \

ENUM_BLOB(cache_control_http, CACHE_CONTROL_HTTP)

// just ignore threading as this should only be used by a single thread in a
// process.  It probably doesn't matter if the token changes anyway.  Worst
// case seems like a partially modified value which would fail and be retried.
typedef int (* access_token_refresh_fn)(blob_t * token);

#define ACCESS_TOKEN_SIZE_HTTP 256

void
init_http()
{
    curl_global_init(CURL_GLOBAL_ALL);

//    debug(curl_version());

    init_method_http();
    init_cache_control_http();
}


bool
ok_http(s64 response_code)
{
    return response_code >= 200 && response_code < 300;
}


size_t
write_blob_http(void *buf, size_t size, size_t nmemb, void *userp)
{
    assert(size == 1);

    blob_t * response = userp;

    if (available_blob(response) < nmemb) {
        // need a better error for this as curl error is something like "Failed
        // writing body (0 != 297)"  maybe it's better if it just truncates?
        return 0;
    }

    ssize_t written = write_blob(response, buf, nmemb);
    return (written == -1) ? 0 : (size_t)written;
}

unsigned int
exponential_backoff(unsigned int min_s)
{
    // TODO(jason): implement;)
    return min_s;
}


long
post_json_http(const blob_t * url, blob_t * response, blob_t * body, blob_t * access_token, access_token_refresh_fn token_refresh)
{
    assert(url->size < MAX_SIZE_URL);

    long response_code = 0;

    CURL * curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, cstr_blob(url));

//        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        if (response) {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_blob_http);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        }

        char error[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

        struct curl_slist * headers = NULL;
        // TODO(jason): is this accept header necessary?
        headers = curl_slist_append(headers, "accept: application/json");
        headers = curl_slist_append(headers, "content-type: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (body) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)body->data);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body->size);
        }

        // google recommends 1 retry, assuming non-auth failures like 503, 429
        int attempts = 2;
        unsigned int sleep_s = 0;
        // no spec constraint on oauth 2 access token size
        //char token[2048];
        do {
            if (sleep_s) sleep(sleep_s);

            if (access_token && empty_blob(access_token) && token_refresh) {
                // if there's a failure getting the access token it will set it to
                // null which should just disable it in curl
                token_refresh(access_token);
                //read_blob(access_token, token, 2048);
            }

            if (valid_blob(access_token)) {
                curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, cstr_blob(access_token));
                curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
            }

            CURLcode res = curl_easy_perform(curl);
            if (res) {
                error_log(error, "curl error", res);
                break;
            }

            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            // auth failure doesn't count as an attempt
            if (response_code == 401) {
                if (token_refresh) {
                    erase_blob(access_token);
                    sleep_s = 0;
                } else {
                    // if can't refresh token then fail immediately
                    attempts = 0;
                }
            }
            else if (response_code == 429) {
                // google rec exp back min 30s
                sleep_s = exponential_backoff(30);
                attempts--;
            }
            else if (response_code == 503) {
                // google rec exp back min 1s
                sleep_s = exponential_backoff(1);
                attempts--;
            }
            else {
                attempts = 0;
            }
        } while (attempts);

        curl_slist_free_all(headers);

        curl_easy_cleanup(curl);
    }

    return response_code;
}


error_t
head_http(const blob_t * url, long * status_code)
{
    assert(url->size < MAX_SIZE_URL);

    CURL * curl = curl_easy_init();

    if (!curl) return -1;

    curl_easy_setopt(curl, CURLOPT_URL, cstr_blob(url));
    curl_easy_setopt(curl,  CURLOPT_NOBODY, 1L);

    char error[CURL_ERROR_SIZE];
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

    long response_code = -1;

    CURLcode res = curl_easy_perform(curl);
    if (res) {
        error_log(error, "http", res);
    }
    else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (status_code) {
            *status_code = response_code;
        }
    }

    curl_easy_cleanup(curl);

    return ok_http(response_code) ? 0 : response_code;
}


int
get_http(const blob_t * url, blob_t * response)
{
    assert(url->size < MAX_SIZE_URL);

    CURL * curl = curl_easy_init();

    if (!curl) return -1;

    int rc = 0;

    curl_easy_setopt(curl, CURLOPT_URL, cstr_blob(url));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_blob_http);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    char error[CURL_ERROR_SIZE];
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

    CURLcode res = curl_easy_perform(curl);
    if (res) {
        error_log(error, "http", res);
        rc = -1;
    }

    curl_easy_cleanup(curl);

    return rc;
}


/*
int
get_fd_http(const blob_t * url, int output_fd, long * response_code)
{
    return 0;
}


int
put_fd_http(const blob_t * url, int input_fd, long * response_code)
{
    return 0;
}
*/

size_t
read_fd_http(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    int fd = *((int *)userdata);
//    debug_s64(fd);

    ssize_t size_read = read(fd, ptr, size*nmemb);
    return (size_read == -1) ? CURL_READFUNC_ABORT : (size_t)size_read;
}


size_t
write_fd_http(void *buf, size_t size, size_t nmemb, void *userdata)
{
    assert(size == 1);

    int fd = *((int *)userdata);
    ssize_t size_write = write(fd, buf, nmemb);
    return (size_write == -1) ? 0 : (size_t)size_write;
}


int
put_file_http(const blob_t * url, const blob_t * path, const blob_t * content_type, cache_control_http_t cache_control, long * status_code)
{
//    debug_blob(url);
//    debug_blob(path);

    dev_error(content_type == NULL);

    int fd = open_read_file_fu(path);
    if (fd == -1) return -1;

    size_t size_fd = 0;
    if (size_file_fu(fd, &size_fd)) return -1;

//    debug_s64(size_fd);

    CURL * curl = curl_easy_init();
    if (!curl) {
        error_log("curl_easy_init failed", "http", 1);
        return -1;
    }

//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, 1024*1024L);

    curl_easy_setopt(curl, CURLOPT_URL, cstr_blob(url));

    char error[CURL_ERROR_SIZE];
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_fd_http);
    curl_easy_setopt(curl, CURLOPT_READDATA, &fd);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)size_fd);

    struct curl_slist * headers = NULL;
    // TODO(jason): is this accept header necessary?
//    headers = curl_slist_append(headers, "accept: application/json");
    blob_t * content_type_header = stk_blob(255);
    vadd_blob(content_type_header, B("content-type:"), content_type);
    headers = curl_slist_append(headers, cstr_blob(content_type_header));
    // TODO(jason): make optional?  probably
    if (cache_control != unknown_cache_control_http) {
        blob_t * cc = stk_blob(255);
        vadd_blob(cc, B("cache-control:"), blob_cache_control_http(cache_control));
        headers = curl_slist_append(headers, cstr_blob(cc));
    }
    // TODO(jason): maybe add later as an option.  with versioning, it seems
    // unlikely to matter.
//    headers = curl_slist_append(headers, "if-none-match:*");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // TODO(jason): how should the file be reset between attempts?

    int attempts = 2;
    unsigned int sleep_s = 0;
    long response_code = 0;
    do {
        if (sleep_s) sleep(sleep_s);

        CURLcode res = curl_easy_perform(curl);
        if (res) {
            error_log(error, "http", res);
            break;
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        // auth failure doesn't count as an attempt
        if (response_code == 429) {
            // google rec exp back min 30s
            sleep_s = exponential_backoff(30);
            attempts--;
        }
        else if (response_code == 503) {
            // google rec exp back min 1s
            sleep_s = exponential_backoff(1);
            attempts--;
        }
        else {
            attempts = 0;
        }
    } while (attempts);

    curl_slist_free_all(headers);

    if (status_code) {
        *status_code = response_code;
    }

    close(fd);

//    debug_s64(response_code);

    return ok_http(response_code) ? 0 : response_code;
}


error_t
get_file_http(const blob_t * url, const blob_t * path, long * status_code)
{
    assert(url->size < MAX_SIZE_URL);

    CURL * curl = curl_easy_init();

    if (!curl) return -1;

    int fd = open_write_file_fu(path);
    if (fd == -1) return -1;

//    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 1024*1024L);

    curl_easy_setopt(curl, CURLOPT_URL, cstr_blob(url));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_fd_http);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fd);

    char error[CURL_ERROR_SIZE];
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

    CURLcode res = curl_easy_perform(curl);
    close(fd);

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
//    debug_s64(response_code);
    if (status_code) {
        *status_code = response_code;
    }

//    debug_s64(res);
    if (res) {
        error_log(error, "http", res);
        // NOTE(jason): allows commenting out the delete for debugging
        // NOTE(jason): delete since file can be an XML response or other error page
        delete_file(path);
    }

    curl_easy_cleanup(curl);

    return res ? -1 : 0;
}

