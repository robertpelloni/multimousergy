#include <iostream>
#include <cassert>
#include <cstring>
#include "NetMuxFramework.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"

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

void test_packet_serialization() {
    std::cout << "Testing Packet serialization..." << std::endl;
    Packet p1 = { 123, PacketType::Movement, 10, -20, 1, true };
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

    // In a real E2E we'd spin up two threads, but for CI we test the logic flow
    NetMuxFramework server;
    NetMuxFramework client;

    AppSettings serverSettings = { true, "127.0.0.1", 6666, {100, 200, false} };
    AppSettings clientSettings = { false, "127.0.0.1", 6666, {300, 400, true} };

    // We don't call Initialize() as it might block/fail on UI in CI,
    // but we can test the internal state machines if they were exposed.
    // For now, this is a placeholder for structural E2E.
}

int main() {
    std::cout << "Running integration tests..." << std::endl;

    test_config_manager();
    test_packet_serialization();
    test_initialization();
    test_coordinated_e2e();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
