#pragma once
#include <string>
#include <vector>

class AuthModule {
public:
    // Computes SHA-256 hash of data. outHash must be at least 32 bytes.
    static bool ComputeSHA256(const std::string& data, unsigned char* outHash);

    // Converts a 32-byte hash to a hex string
    static std::string HashToHex(const unsigned char* hash);

    // Generates a response for a given nonce and key
    static void GenerateResponse(int nonce, const std::string& key, unsigned char* outHash);

    // Verifies if a received hash matches the expected response for a nonce/key
    static bool VerifyResponse(int nonce, const std::string& key, const unsigned char* receivedHash);
};
