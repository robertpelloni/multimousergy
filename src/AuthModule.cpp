#include "AuthModule.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

struct WinSHA256Internal {
    BCRYPT_ALG_HANDLE hAlg;
    BCRYPT_HASH_HANDLE hHash;
    PBYTE pbHashObject;
};
#else
#include <openssl/sha.h>
#endif

bool AuthModule::ComputeSHA256(const std::string& data, unsigned char* outHash) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbData = 0, cbHash = 0, cbHashObject = 0;
    PBYTE pbHashObject = NULL;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) return false;

    if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0) < 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) < 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
    if (NULL == pbHashObject) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0) < 0) {
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (BCryptHashData(hHash, (PBYTE)data.c_str(), (ULONG)data.length(), 0) < 0) {
        BCryptDestroyHash(hHash);
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (BCryptFinishHash(hHash, outHash, 32, 0) < 0) {
        BCryptDestroyHash(hHash);
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCryptDestroyHash(hHash);
    HeapFree(GetProcessHeap(), 0, pbHashObject);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return true;
#else
    SHA256((const unsigned char*)data.c_str(), data.length(), outHash);
    return true;
#endif
}

AuthModule::SHA256Context AuthModule::CreateContext() {
    SHA256Context ctx = { nullptr };
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbData = 0, cbHashObject = 0;
    PBYTE pbHashObject = NULL;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) return ctx;
    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) < 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return ctx;
    }
    pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
    if (BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0) < 0) {
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return ctx;
    }
    auto internal = new WinSHA256Internal();
    internal->hAlg = hAlg;
    internal->hHash = hHash;
    internal->pbHashObject = pbHashObject;
    ctx.internal = internal;
#else
    auto shaCtx = new SHA256_CTX();
    SHA256_Init(shaCtx);
    ctx.internal = shaCtx;
#endif
    return ctx;
}

bool AuthModule::UpdateHash(SHA256Context& ctx, const void* data, size_t len) {
    if (!ctx.internal) return false;
#ifdef _WIN32
    auto internal = (WinSHA256Internal*)ctx.internal;
    return BCryptHashData(internal->hHash, (PBYTE)data, (ULONG)len, 0) >= 0;
#else
    return SHA256_Update((SHA256_CTX*)ctx.internal, data, len) == 1;
#endif
}

bool AuthModule::FinalizeHash(SHA256Context& ctx, unsigned char* outHash) {
    if (!ctx.internal) return false;
    bool success = false;
#ifdef _WIN32
    auto internal = (WinSHA256Internal*)ctx.internal;
    success = BCryptFinishHash(internal->hHash, outHash, 32, 0) >= 0;
    BCryptDestroyHash(internal->hHash);
    HeapFree(GetProcessHeap(), 0, internal->pbHashObject);
    BCryptCloseAlgorithmProvider(internal->hAlg, 0);
    delete internal;
#else
    success = SHA256_Final(outHash, (SHA256_CTX*)ctx.internal) == 1;
    delete (SHA256_CTX*)ctx.internal;
#endif
    ctx.internal = nullptr;
    return success;
}

std::string AuthModule::HashToHex(const unsigned char* hash) {
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void AuthModule::GenerateResponse(int nonce, const std::string& key, unsigned char* outHash) {
    std::string data = std::to_string(nonce) + key;
    ComputeSHA256(data, outHash);
}

bool AuthModule::VerifyResponse(int nonce, const std::string& key, const unsigned char* receivedHash) {
    unsigned char expectedHash[32];
    GenerateResponse(nonce, key, expectedHash);

    // Constant-time comparison to mitigate timing attacks
    unsigned char result = 0;
    for (int i = 0; i < 32; ++i) {
        result |= (expectedHash[i] ^ receivedHash[i]);
    }
    return result == 0;
}
