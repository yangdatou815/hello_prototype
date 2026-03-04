#include "Crypto.hpp"
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <vector>
#include <stdexcept>
#include <cstring>

// ─────────────────────────────────────────────────────────────
// Shared key / IV  (prototype: hardcoded, 256-bit / 128-bit)
// Replace with a proper key-exchange (DH / TLS) in production.
// ─────────────────────────────────────────────────────────────
const unsigned char Crypto::KEY[32] = {
    0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f
};

const unsigned char Crypto::IV[16] = {
    0xa0,0xa1,0xa2,0xa3, 0xa4,0xa5,0xa6,0xa7,
    0xa8,0xa9,0xaa,0xab, 0xac,0xad,0xae,0xaf
};

// ─────────────────────────────────────────────────────────────
// Base64 encode (OpenSSL BIO)
// ─────────────────────────────────────────────────────────────
std::string Crypto::base64_encode(const unsigned char* data, size_t len) {
    BIO* b64  = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // no newlines
    BIO_write(b64, data, static_cast<int>(len));
    BIO_flush(b64);

    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

// ─────────────────────────────────────────────────────────────
// Base64 decode (OpenSSL BIO)
// ─────────────────────────────────────────────────────────────
std::string Crypto::base64_decode(const std::string& b64) {
    BIO* b64bio = BIO_new(BIO_f_base64());
    BIO* bmem   = BIO_new_mem_buf(b64.data(), static_cast<int>(b64.size()));
    bmem = BIO_push(b64bio, bmem);
    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);

    std::vector<unsigned char> buf(b64.size());
    int decoded_len = BIO_read(bmem, buf.data(), static_cast<int>(b64.size()));
    BIO_free_all(bmem);

    if (decoded_len < 0) {
        throw std::runtime_error("[Crypto] Base64 decode failed.");
    }
    return std::string(reinterpret_cast<char*>(buf.data()), decoded_len);
}

// ─────────────────────────────────────────────────────────────
// AES-256-CBC encrypt → Base64
// ─────────────────────────────────────────────────────────────
std::string Crypto::encrypt(const std::string& plaintext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("[Crypto] EVP_CIPHER_CTX_new failed.");

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, KEY, IV) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("[Crypto] EncryptInit failed.");
    }

    // Output buffer: plaintext + one full AES block (16 bytes) for padding
    std::vector<unsigned char> ciphertext(plaintext.size() + 16);
    int len = 0, total = 0;

    if (EVP_EncryptUpdate(ctx,
            ciphertext.data(), &len,
            reinterpret_cast<const unsigned char*>(plaintext.data()),
            static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("[Crypto] EncryptUpdate failed.");
    }
    total = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("[Crypto] EncryptFinal failed.");
    }
    total += len;
    EVP_CIPHER_CTX_free(ctx);

    return base64_encode(ciphertext.data(), total);
}

// ─────────────────────────────────────────────────────────────
// Base64 → AES-256-CBC decrypt
// ─────────────────────────────────────────────────────────────
std::string Crypto::decrypt(const std::string& b64_ciphertext) {
    std::string ciphertext = base64_decode(b64_ciphertext);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("[Crypto] EVP_CIPHER_CTX_new failed.");

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, KEY, IV) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("[Crypto] DecryptInit failed.");
    }

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0, total = 0;

    if (EVP_DecryptUpdate(ctx,
            plaintext.data(), &len,
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("[Crypto] DecryptUpdate failed.");
    }
    total = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + total, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        // Collect OpenSSL error string
        unsigned long err = ERR_get_error();
        char errbuf[256];
        ERR_error_string_n(err, errbuf, sizeof(errbuf));
        throw std::runtime_error(std::string("[Crypto] DecryptFinal failed: ") + errbuf);
    }
    total += len;
    EVP_CIPHER_CTX_free(ctx);

    return std::string(reinterpret_cast<char*>(plaintext.data()), total);
}
