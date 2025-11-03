#pragma once

#include <openssl/err.h>
#include <openssl/ssl.h>


typedef struct {
    error_t error;

    SSL * _ssl;
} tls_t;


struct {
    SSL_CTX * ssl_ctx;
} _tls;


#define error_log_tls(void) \
    _error_log_tls(__FILE__, __func__, __LINE__);

void
_error_log_tls(const char * file, const char * function, s64 line)
{
    unsigned long err = 0;
    while ((err = ERR_get_error())) {
        const char * reason = ERR_reason_error_string(err);
        _log(plog, reason, strlen(reason), "tls", 3, err, file, function, line);
    }
}


error_t
init_tls(const blob_t * hostname)
{
    /*
     * An SSL_CTX holds shared configuration information for multiple
     * subsequent per-client SSL connections.
     */
    SSL_CTX * ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == NULL) {
//        ERR_print_errors_fp(stderr);
        error_log_tls();
        fail_log("Failed to create server SSL_CTX");
        exit(EXIT_FAILURE);
    }

    /*
     * TLS versions older than TLS 1.2 are deprecated by IETF and SHOULD
     * be avoided if possible.
     */
    if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
        SSL_CTX_free(ctx);
//        ERR_print_errors_fp(stderr);
        error_log_tls();
        fail_log("Failed to set the minimum TLS protocol version");
        exit(EXIT_FAILURE);
    }

    SSL_CTX_set_security_level(ctx, 3);

    /*
     * Tolerate clients hanging up without a TLS "shutdown".  Appropriate in all
     * application protocols which perform their own message "framing", and
     * don't rely on TLS to defend against "truncation" attacks.
     */
    long opts = SSL_OP_IGNORE_UNEXPECTED_EOF;

    /*
     * Block potential CPU-exhaustion attacks by clients that request frequent
     * renegotiation.  This is of course only effective if there are existing
     * limits on initial full TLS handshake or connection rates.
     */
    opts |= SSL_OP_NO_RENEGOTIATION;

    /*
     * Most servers elect to use their own cipher or group preference rather
     * than that of the client.
     */
    opts |= SSL_OP_CIPHER_SERVER_PREFERENCE;

    /* Apply the selection options */
    SSL_CTX_set_options(ctx, opts);


    bool use_lets_encrypt = false;
    if (use_lets_encrypt) {
        // get from ACME (let's encrypt, etc)
    }
    else {
        blob_t * cert_pem = new_state_path_app(B("cert.pem"));
        blob_t * key_pem = new_state_path_app(B("key.pem"));

        debug_blob(cert_pem);
        debug_blob(key_pem);

        // TODO(jason): check localhost, *.local, etc hostnames for self signed cert

        if (SSL_CTX_use_certificate_file(ctx, cstr_blob(cert_pem), SSL_FILETYPE_PEM) != 1
            || SSL_CTX_use_PrivateKey_file(ctx, cstr_blob(key_pem), SSL_FILETYPE_PEM) != 1)
        {
            error_log_tls();

            EVP_PKEY *pkey = EVP_EC_gen("P-256");

            debug("generated private key");

            X509 * x509 = X509_new();
            X509_set_version(x509, 2); // X.509 v3
            ASN1_INTEGER_set(X509_get_serialNumber(x509), 1); // Set serial number

            X509_NAME *name = X509_get_subject_name(x509);
            //        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"US", -1, -1, 0);
            //        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)cstr_blob(hostname), -1, -1, 0);
            X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cstr_blob(hostname), -1, -1, 0);
            X509_set_issuer_name(x509, name);

            X509_set_pubkey(x509, pkey);

            X509_gmtime_adj(X509_get_notBefore(x509), 0); // Now
            X509_gmtime_adj(X509_get_notAfter(x509), 365 * 24 * 60 * 60); // 1 year validity

            X509_sign(x509, pkey, EVP_sha256()); // Sign with private key and SHA256
            debug("signed certificate");

            SSL_CTX_use_certificate(ctx, x509);
            SSL_CTX_use_PrivateKey(ctx, pkey);

            BIO * bio = BIO_new_file(cstr_blob(cert_pem), "w");
            PEM_write_bio_X509(bio, x509);
            if (bio) BIO_free_all(bio);

            bio = BIO_new_file(cstr_blob(key_pem), "w");
            PEM_write_bio_PKCS8PrivateKey(bio, pkey, NULL, NULL, 0, NULL, NULL);
            if (bio) BIO_free_all(bio);

            debug("cert written");
        }

        free_blob(cert_pem);
        free_blob(key_pem);
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    error_log_tls();

    _tls.ssl_ctx = ctx;

    return 0;
}


tls_t *
new_server_tls()
{
    tls_t * tls = malloc(sizeof(*tls));
    if (!tls) return NULL;

    tls->error = 0;
    tls->_ssl = NULL;

    return tls;
}


error_t
reset_tls(tls_t * tls, int client_fd)
{
    dev_error(tls == NULL);

    tls->error = 0;

    ERR_clear_error();

    if (tls->_ssl) {
        SSL_free(tls->_ssl);
        tls->_ssl = NULL;
    }

    tls->_ssl = SSL_new(_tls.ssl_ctx);

    SSL_set_fd(tls->_ssl, client_fd);

    return -1;
}


error_t
accept_tls(tls_t * tls, int client_fd)
{
    // TODO(jason): this should probably just set the client fd and not reset
    // since reset should happen at the end of a request
    reset_tls(tls, client_fd);

    return (SSL_accept(tls->_ssl) == 1) ? 0 : -1;
}


void
free_tls(tls_t * tls)
{
    SSL_free(tls->_ssl);
    free(tls);
}


error_t
shutdown_tls(tls_t * tls)
{
    SSL_shutdown(tls->_ssl);
    SSL_free(tls->_ssl);
    tls->_ssl = NULL;

    return 0;
}


ssize_t
write_tls(tls_t * tls, const blob_t * buf)
{
    // TODO(jason): revisit return values
    return SSL_write(tls->_ssl, buf->data, buf->size);
}


ssize_t
read_tls(tls_t * tls, blob_t * blob)
{
    SSL * ssl = tls->_ssl;

    size_t available = available_blob(blob);
    u8 * buf = &blob->data[blob->size];

    int rc = SSL_read(ssl, buf, available);
    if (rc > 0) {
        blob->size += rc;
        return rc;
    }
    else {
        error_log_tls();
        // TODO(jason): probably wrong, but i'm tired of looking into openssl error handling
        return rc < 0 ? -1 : 0;
    }
}


//ssize_t
//send_file_tls(tls_t * tls, int fd, size_t len)
//{
//    ssize_t written = SSL_sendfile(tls->_ssl, fd, 0, len, 0);
//    if (written < 0) {
//        error_log_tls();
//        return -1;
//    }
//    else {
//        return written;
//    }
//}

