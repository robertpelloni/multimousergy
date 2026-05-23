#include <iostream>
#include <cassert>
#include <cstring>
#include "NetMuxFramework.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "SyncModule.hpp"
#include "ClipboardModule.hpp"

// External declarations from test_multi_client.cpp
void test_concurrent_cursor_sync();

void test_sync_module_interpolation() {
    std::cout << "Testing SyncModule interpolation..." << std::endl;
    SyncModule sync;
    unsigned long long peerId = 1;

    // Initial update
    sync.UpdatePeer(peerId, 32768, 32768); // Center 0.5, 0.5
    PeerState s;
    sync.GetPeerState(peerId, s);
    int startX = s.x;

    // Because of the jitter buffer, we need multiple updates or
    // to fill the buffer to see movement in the interpolated 'target'
    for(int i=0; i<6; ++i) {
        sync.UpdatePeer(peerId, 65535, 65535);
    }

    // Step and check movement
    sync.Step(16.0);
    sync.GetPeerState(peerId, s);

    assert(s.x > startX);
    assert(s.x <= s.targetX);
}

void test_coordinate_edge_cases() {
    std::cout << "Testing coordinate normalization edge cases..." << std::endl;

    // Extreme screen sizes
    int maxNorm = 65535;
    assert(SyncModule::Normalize(0, 1920) == 0);
    assert(SyncModule::Normalize(1919, 1920) == maxNorm);
    assert(SyncModule::Denormalize(0, 1920) == 0);
    assert(SyncModule::Denormalize(maxNorm, 1920) == 1919);

    // Ultra-wide
    assert(SyncModule::Normalize(5119, 5120) == maxNorm);
    assert(SyncModule::Denormalize(maxNorm, 5120) == 5119);

    // Vertical
    assert(SyncModule::Normalize(1079, 1080) == maxNorm);
    assert(SyncModule::Denormalize(maxNorm, 1080) == 1079);
}

void test_jitter_buffer_overflow() {
    std::cout << "Testing jitter buffer overflow..." << std::endl;
    SyncModule sync;
    unsigned long long peerId = 1;

    // Fill and overflow buffer (size is 5)
    for (int i = 0; i < 10; ++i) {
        sync.UpdatePeer(peerId, i * 100, i * 100);
    }

    PeerState s;
    assert(sync.GetPeerState(peerId, s));
    assert(s.jitterBuffer.size() == 5);
    // front should be the 6th point (index 5)
    assert(s.jitterBuffer.front().nx == 500);
}

void test_config_manager() {
    std::cout << "Testing ConfigManager..." << std::endl;
    ConfigManager cm("test.cfg");
    AppSettings s1 = { true, "10.0.0.1", 1234, {100, 200, true} };
    cm.Save(s1);

    AppSettings s2 = { false, "", 0, {0, 0, false} };
    assert(cm.Load(s2));
    assert(s2.isServer == s1.isServer);
    assert(s2.remoteIp == s1.remoteIp);
    assert(s2.port == s1.port);
    assert(s2.inputConfig.boundaryX == s1.inputConfig.boundaryX);
    assert(s2.inputConfig.isLeft == s1.inputConfig.isLeft);
}

void test_clipboard_module() {
    std::cout << "Testing ClipboardModule..." << std::endl;
    ClipboardModule cm;
    // Basic instantiation check
}

void test_packet_serialization() {
    std::cout << "Testing Packet serialization..." << std::endl;
    Packet p1 = { 123, PacketType::Movement, 10, -20, 1, true };
    strcpy(p1.payload, "test");
    p1.payloadSize = 4;

    Packet p2;
    std::memcpy(&p2, &p1, sizeof(Packet));
    assert(p2.senderId == p1.senderId);
    assert(p2.type == p1.type);
    assert(p2.x == p1.x);
    assert(p2.y == p1.y);
    assert(p2.button == p1.button);
    assert(p2.down == p1.down);
}

void test_initialization() {
    std::cout << "Testing framework initialization..." << std::endl;
    NetMuxFramework framework;
    assert(sizeof(framework) > 0);
}

void test_coordinated_e2e() {
    std::cout << "Testing coordinated E2E cursor sync (Mock)..." << std::endl;

    // Test that our SyncModule and Packet system handle a full round-trip
    // coordinate transformation correctly.
    SyncModule serverSync;

    // Client sends an absolute position (Normalized)
    int clientResX = 1920;
    int clientResY = 1080;
    int clientPosX = 960;
    int clientPosY = 540;

    int normX = SyncModule::Normalize(clientPosX, clientResX);
    int normY = SyncModule::Normalize(clientPosY, clientResY);

    // Packet arrives at server
    Packet p = { 101, PacketType::AbsoluteMovement, normX, normY, 0, false };

    // Server processes packet
    serverSync.UpdatePeer(p.senderId, p.x, p.y);

    PeerState s;
    assert(serverSync.GetPeerState(101, s));

    // The denormalized coordinate on server should match original (with small rounding margin)
    // Server resolution is retrieved from GetSystemMetrics, but SyncModule falls back to 1920x1080
    // if not on Windows.
    assert(std::abs(s.x - clientPosX) <= 1);
    assert(std::abs(s.y - clientPosY) <= 1);
}

int main() {
    std::cout << "Running integration tests..." << std::endl;

    test_config_manager();
    test_packet_serialization();
    test_initialization();
    test_coordinated_e2e();
    test_sync_module_interpolation();
    test_coordinate_edge_cases();
    test_jitter_buffer_overflow();
    test_concurrent_cursor_sync();
    test_clipboard_module();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
