#pragma once

// NOTE(jason): started with https://curl.se/libcurl/c/smtp-ssl.html

#include <curl/curl.h>

struct {
    blob_t * server;
    blob_t * sender;
} _config_smtp;


// TODO(jason): maybe this should only be a sqlite db as parameter
int
init_smtp(blob_t * server, blob_t * sender)
{
    _config_smtp.server = copy_blob(server);
    _config_smtp.sender = copy_blob(sender);

    return curl_global_init(CURL_GLOBAL_ALL);
}


typedef struct {
    blob_t * message;
    ssize_t offset;
} read_state_smtp_t;


size_t
read_message_smtp(char *buffer, size_t size, size_t nitems, void *userdata)
{
    read_state_smtp_t * read_state = userdata;
    blob_t * msg = read_state->message;
    ssize_t offset = read_state->offset;

    ssize_t bytes_read = read_blob(msg, offset, buffer, size*nitems);
    if (bytes_read < 0) return CURL_READFUNC_ABORT;

    read_state->offset += bytes_read;

    return bytes_read;
}


int
send_smtp(const blob_t * email, const blob_t * subject, const blob_t * body)
{
    CURL * curl = curl_easy_init();
    if (!curl) {
        error_log("curl init failed", "curl", 1);
        return -1;
    }

    CURLcode res = CURLE_OK;

//    curl_easy_setopt(curl, CURLOPT_USERNAME, "user");
//    curl_easy_setopt(curl, CURLOPT_PASSWORD, "secret");

    blob_t * url = stk_blob(1024);
    add_blob(url, B("smtps://"));
    add_blob(url, _config_smtp.server);

    /* This is the URL for your mailserver. Note the use of smtps:// rather
     * than smtp:// to request a SSL based connection. */
    curl_easy_setopt(curl, CURLOPT_URL, cstr_blob(url));

    blob_t * from_email = stk_blob(1024);
    write_blob(from_email, "<", 1);
    add_blob(from_email, _config_smtp.sender);
    write_blob(from_email, ">", 1);

    blob_t * msg = stk_blob(body->size + 1024);
    add_blob(msg, B("from:"));
    add_blob(msg, from_email);
    write_blob(msg, "\r\n", 2);
    add_blob(msg, B("to:"));
    add_blob(msg, email);
    write_blob(msg, "\r\n", 2);
    add_blob(msg, B("subject:"));
    add_blob(msg, subject);
    write_blob(msg, "\r\n", 2);
    // NOTE(jason): blank line between header and body
    write_blob(msg, "\r\n", 2);
    add_blob(msg, body);
    //debug_blob(msg);

    read_state_smtp_t read_state = { .message = msg, .offset = 0 };

    /* Note that this option is not strictly required, omitting it results in
     * libcurl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, cstr_blob(from_email));

    struct curl_slist * recipients = NULL;
    recipients = curl_slist_append(recipients, cstr_blob(email));
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_message_smtp);
    curl_easy_setopt(curl, CURLOPT_READDATA, &read_state);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* Since the traffic is encrypted, it is useful to turn on debug
     * information within libcurl to see what is happening during the
     * transfer */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        error_log(curl_easy_strerror(res), "curl", res);
    }

    curl_slist_free_all(recipients);

    curl_easy_cleanup(curl);

    return 0;
}

