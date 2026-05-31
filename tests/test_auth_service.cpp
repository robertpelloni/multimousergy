#include <iostream>
#include <cassert>
#include <cstring>
#include "AuthService.hpp"
#include "AuthModule.hpp"

void test_auth_service_flow() {
    AuthService service;
    unsigned long long peerId = 42;
    std::string key = "password123";

    // 1. Create challenge
    int nonce = service.CreateChallenge(peerId);
    assert(nonce != 0);

    // 2. Generate valid response
    unsigned char hash[32];
    AuthModule::GenerateResponse(nonce, key, hash);

    // 3. Verify valid response
    bool verified = service.VerifyResponse(peerId, key, hash);
    assert(verified == true);

    // 4. Verify nonce was consumed (one-time use)
    verified = service.VerifyResponse(peerId, key, hash);
    assert(verified == false);

    std::cout << "[Test] AuthService flow passed." << std::endl;
}

void test_auth_service_invalid_peer() {
    AuthService service;
    std::string key = "password123";
    unsigned char dummyHash[32] = {0};

    bool verified = service.VerifyResponse(999, key, dummyHash);
    assert(verified == false);

    std::cout << "[Test] AuthService invalid peer check passed." << std::endl;
}

void test_auth_service_clear_peer() {
    AuthService service;
    unsigned long long peerId = 42;
    service.CreateChallenge(peerId);
    service.ClearPeer(peerId);

    unsigned char dummyHash[32] = {0};
    bool verified = service.VerifyResponse(peerId, "any", dummyHash);
    assert(verified == false);

    std::cout << "[Test] AuthService clear peer passed." << std::endl;
}

void test_auth_service() {
    std::cout << "Running AuthService Tests..." << std::endl;
    test_auth_service_flow();
    test_auth_service_invalid_peer();
    test_auth_service_clear_peer();
    std::cout << "All AuthService Tests Passed!" << std::endl;
}
