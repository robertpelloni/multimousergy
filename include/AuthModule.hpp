#pragma once
#include <string>
#include <vector>

class AuthModule {
public:
    // Computes SHA-256 hash of data. outHash must be at least 32 bytes.
    static bool ComputeSHA256(const std::string& data, unsigned char* outHash);

    // Streaming SHA-256
    struct SHA256Context {
        void* internal;
    };
    static SHA256Context CreateContext();
    static bool UpdateHash(SHA256Context& ctx, const void* data, size_t len);
    static bool FinalizeHash(SHA256Context& ctx, unsigned char* outHash);

    // Converts a 32-byte hash to a hex string
    static std::string HashToHex(const unsigned char* hash);

    // Generates a response for a given nonce and key
    static void GenerateResponse(int nonce, const std::string& key, unsigned char* outHash);

    // Verifies if a received hash matches the expected response for a nonce/key
    static bool VerifyResponse(int nonce, const std::string& key, const unsigned char* receivedHash);
};
