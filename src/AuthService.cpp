#include "AuthService.hpp"
#include "AuthModule.hpp"
#include <chrono>

AuthService::AuthService() {
    std::random_device rd;
    m_rng.seed(rd());
}

AuthService::~AuthService() {}

int AuthService::CreateChallenge(unsigned long long peerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::uniform_int_distribution<int> dist(1, 0x7FFFFFFF);
    int nonce = dist(m_rng);
    m_pendingChallenges[peerId] = nonce;
    return nonce;
}

bool AuthService::VerifyResponse(unsigned long long peerId, const std::string& key, const unsigned char* receivedHash) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pendingChallenges.count(peerId) == 0) return false;

    int nonce = m_pendingChallenges[peerId];
    m_pendingChallenges.erase(peerId); // One-time use

    return AuthModule::VerifyResponse(nonce, key, receivedHash);
}

void AuthService::ClearPeer(unsigned long long peerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingChallenges.erase(peerId);
}
