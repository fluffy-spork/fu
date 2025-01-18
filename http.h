#pragma once

#include <curl/curl.h>

#define MAX_SIZE_URL 1024

// just ignore threading as this should only be used by a single thread in a
// process.  It probably doesn't matter if the token changes anyway.  Worst
// case seems like a partially modified value which would fail and be retried.
typedef int (* access_token_refresh_fn)(blob_t * token);

#define ACCESS_TOKEN_SIZE_HTTP 256

void
init_http()
{
    curl_global_init(CURL_GLOBAL_ALL);
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
        char s[MAX_SIZE_URL];
        read_cstr_blob(url, 0, s, MAX_SIZE_URL);
        curl_easy_setopt(curl, CURLOPT_URL, s);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        if (response) {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_blob_http);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        }

        char error[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

        struct curl_slist * headers = NULL;
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

void
get_http(const blob_t * url, blob_t * response)
{
    assert(url->size < MAX_SIZE_URL);

    CURL * curl = curl_easy_init();
    if (curl) {
        char s[MAX_SIZE_URL];
        CURLcode res;
        read_cstr_blob(url, 0, s, MAX_SIZE_URL);
        curl_easy_setopt(curl, CURLOPT_URL, s);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_blob_http);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        char error[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

        res = curl_easy_perform(curl);
        if (res) {
            error_log(error, "curl_easy_perform", res);
        }

        curl_easy_cleanup(curl);
    }
}

