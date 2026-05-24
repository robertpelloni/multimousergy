#include <iostream>
#include <cassert>
#include <cstring>
#include "NetworkManager.hpp"

void test_packet_serialization_integrity() {
    std::cout << "Testing Network Packet serialization integrity..." << std::endl;
    Packet p1;
    p1.senderId = 0xABCDEF1234567890ULL;
    p1.groupId = 42;
    p1.type = PacketType::AbsoluteMovement;
    p1.x = 32767;
    p1.y = 16383;
    p1.localTimestamp = 1234567.89;

    char buffer[sizeof(Packet)];
    memcpy(buffer, &p1, sizeof(Packet));

    Packet p2;
    memcpy(&p2, buffer, sizeof(Packet));

    assert(p2.senderId == p1.senderId);
    assert(p2.groupId == p1.groupId);
    assert(p2.type == p1.type);
    assert(p2.x == p1.x);
    assert(p2.y == p1.y);
    assert(p2.localTimestamp == p1.localTimestamp);
}

void test_network_discovery_logic() {
    std::cout << "Testing Discovery Packet logic..." << std::endl;
    DiscoveryPacket dp;
    std::string mockHost = "MockHost";
    strncpy(dp.hostname, mockHost.c_str(), sizeof(dp.hostname));
    dp.port = 1234;

    assert(std::string(dp.hostname) == "MockHost");
    assert(dp.port == 1234);
}
