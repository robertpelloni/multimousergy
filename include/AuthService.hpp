#pragma once
#include <string>
#include <map>
#include <mutex>
#include <random>

class AuthService {
public:
    AuthService();
    ~AuthService();

    // Creates a new challenge nonce for a peer
    int CreateChallenge(unsigned long long peerId);

    // Verifies the response for a peer
    bool VerifyResponse(unsigned long long peerId, const std::string& key, const unsigned char* receivedHash);

    // Clears state for a peer (e.g. on disconnect)
    void ClearPeer(unsigned long long peerId);

private:
    std::map<unsigned long long, int> m_pendingChallenges;
    std::mutex m_mutex;
    std::mt19937 m_rng;
};
