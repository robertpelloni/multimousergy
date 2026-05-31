#include <iostream>
#include <cassert>
#include <cstring>
#include "AuthModule.hpp"

void test_sha256_known_input() {
    unsigned char hash[32];
    // Known SHA-256 for "abc"
    // ba7816bf 8f01cfea 414140de 5dae2223 b00361a3 96177a9c b410ff61 f20015ad
    AuthModule::ComputeSHA256("abc", hash);
    std::string hex = AuthModule::HashToHex(hash);
    assert(hex == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    std::cout << "[Test] SHA-256 known input passed." << std::endl;
}

void test_challenge_response_success() {
    int nonce = 12345;
    std::string key = "secret_password";
    unsigned char hash[32];

    AuthModule::GenerateResponse(nonce, key, hash);
    bool verified = AuthModule::VerifyResponse(nonce, key, hash);
    assert(verified == true);
    std::cout << "[Test] Challenge-Response success passed." << std::endl;
}

void test_challenge_response_failure() {
    int nonce = 12345;
    std::string key = "secret_password";
    unsigned char hash[32];

    AuthModule::GenerateResponse(nonce, key, hash);

    // Mismatched key
    bool verified = AuthModule::VerifyResponse(nonce, "wrong_password", hash);
    assert(verified == false);

    // Mismatched nonce
    verified = AuthModule::VerifyResponse(54321, key, hash);
    assert(verified == false);

    std::cout << "[Test] Challenge-Response failure cases passed." << std::endl;
}

void test_auth_module() {
    std::cout << "Running Authentication Module Tests..." << std::endl;
    test_sha256_known_input();
    test_challenge_response_success();
    test_challenge_response_failure();
    std::cout << "All Authentication Tests Passed!" << std::endl;
}
