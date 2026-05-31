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
    return memcmp(expectedHash, receivedHash, 32) == 0;
}
