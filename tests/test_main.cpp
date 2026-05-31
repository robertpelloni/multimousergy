#include <iostream>
#include <cassert>
#include <cstring>
#include "NetMuxFramework.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "SyncModule.hpp"
#include "ClipboardModule.hpp"

void test_concurrent_cursor_sync();
void test_group_isolation();
void test_high_concurrency_sync();
void test_simultaneous_editing_sync();
void test_sync_stress();
void test_network_stress();
void test_packet_serialization_integrity();
void test_network_discovery_logic();
void test_concurrent_packet_handling();
void test_coordinate_normalization_fidelity();
void test_clock_offset_calculation();
void test_simultaneous_edit_resolution();
void test_boundary_logic();
void test_auth_module();
void test_auth_service();
void test_clipboard_module();

void test_sync_module_interpolation() {
    std::cout << "Testing SyncModule interpolation..." << std::endl;
    SyncModule sync;
    unsigned long long peerId = 1;
    sync.UpdatePeer(peerId, 0, 32768, 32768);
    PeerState s;
    sync.GetPeerState(peerId, s);
    int startX = (int)s.x;
    for(int i=0; i<6; ++i) sync.UpdatePeer(peerId, 0, 65535, 65535);
    sync.Step(16.0);
    sync.GetPeerState(peerId, s);
    assert(s.x > startX);
}

void test_config_manager() {
    std::cout << "Testing ConfigManager..." << std::endl;
    ConfigManager cm("test.cfg");
    AppSettings s1 = { true, "10.0.0.1", 1234, {100, 200, true}, 1.5f, false, NetMuxDriverType::Interception, 42, "G", "S", "K" };
    cm.Save(s1);
    AppSettings s2 = { false, "", 0, {0, 0, false}, 1.0f, false, NetMuxDriverType::Auto, 0, "", "", "" };
    assert(cm.Load(s2));
    assert(s2.isServer == s1.isServer);
    assert(s2.groupId == 42);
}

int main() {
    std::cout << "Running unit tests..." << std::endl;
    test_config_manager();
    test_auth_module();
    test_auth_service();
    test_sync_module_interpolation();
    test_concurrent_cursor_sync();
    test_high_concurrency_sync();
    test_simultaneous_editing_sync();
    test_simultaneous_edit_resolution();
    test_coordinate_normalization_fidelity();
    test_clock_offset_calculation();
    test_clipboard_module();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
