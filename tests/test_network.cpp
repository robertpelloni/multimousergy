#include <iostream>
#include <cassert>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>
#include "NetworkManager.hpp"

void test_packet_serialization_integrity() {
    std::cout << "Testing Network Packet serialization integrity..." << std::endl;
    Packet p1;
    p1.senderId = 0xABCDEF1234567890ULL;
    p1.groupId = 42;
    p1.type = NetMuxPacketType::AbsoluteMovement;
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

void test_concurrent_packet_handling() {
    std::cout << "Testing Concurrent Packet Handling..." << std::endl;
    NetworkManager server;
    server.StartServer(9991);
    server.SetIsServer(true);

    const int numClients = 5;
    const int packetsPerClient = 1000;
    std::atomic<int> totalSent(0);

    auto clientTask = [&](int id) {
        NetworkManager client;
        client.SetIsServer(false);
        if (client.Connect("127.0.0.1", 9991)) {
            for (int i = 0; i < packetsPerClient; ++i) {
                Packet p = { (unsigned long long)id, 0, 0, 0.0, NetMuxPacketType::AbsoluteMovement, i, i, 0, false, false, 0, 0, 0, false, 0, 0, 1.0f, "", 0 };
                client.SendPacket(p);
                totalSent++;
            }
        }
        client.Shutdown();
    };

    std::vector<std::thread> clients;
    for (int i = 0; i < numClients; ++i) {
        clients.emplace_back(clientTask, i + 1);
    }

    for (auto& t : clients) t.join();

    // Verify server can drain the buffer without crashing
    Packet p;
    int received = 0;
    while (server.ReceivePacket(p)) {
        received++;
    }

    std::cout << "Sent: " << totalSent << ", Received: " << received << std::endl;
    server.Shutdown();
}
