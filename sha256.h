#pragma once

// TODO(jason): consider tweetnacl or something as it could be dropped in repo,
// public domain

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

error_t
sha256(blob_t * digest, const blob_t * data)
{
    dev_error(available_blob(digest) < SHA256_DIGEST_LENGTH);

    unsigned int size_buf = SHA256_DIGEST_LENGTH;
    u8 buf[size_buf];

    if (SHA256(data->data, data->size, buf)) {
        write_blob(digest, buf, size_buf);
        return 0;
    }

    return -1;
}


error_t
hmac_sha256(blob_t * sig, const blob_t * key, const blob_t * data)
{
    unsigned int size_hmac = 32;
    u8 hmac[size_hmac];

    if (HMAC(EVP_sha256(), key->data, key->size, data->data, data->size, hmac, &size_hmac)) {
        write_blob(sig, hmac, size_hmac);
        return 0;
    }

    return -1;
}

