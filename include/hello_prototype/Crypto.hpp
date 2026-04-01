#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <string>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────
// Crypto: AES-256-CBC encrypt/decrypt + Base64 encode/decode
//
// Usage:
//   std::string cipher  = Crypto::encrypt("Hello, World!");
//   std::string plain   = Crypto::decrypt(cipher);
//
// The shared key and IV are embedded for this prototype.
// In production, use a proper key-exchange protocol (e.g. TLS).
// ─────────────────────────────────────────────────────────────
class Crypto {
public:
    // Encrypt plaintext → Base64(AES-256-CBC ciphertext)
    static std::string encrypt(const std::string& plaintext);

    // Decrypt Base64(AES-256-CBC ciphertext) → plaintext
    static std::string decrypt(const std::string& b64_ciphertext);

private:
    static std::string base64_encode(const unsigned char* data, size_t len);
    static std::string base64_decode(const std::string& b64);

    // Shared 256-bit key and 128-bit IV (prototype: hardcoded)
    static const unsigned char KEY[32];
    static const unsigned char IV[16];
};

#endif // CRYPTO_HPP
