#include <iostream>
#include <cassert>
#include <cstring>
#include "NetMuxFramework.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "SyncModule.hpp"
#include "ClipboardModule.hpp"
#include "WebcamCapture.hpp"
#include "VideoEncoder.hpp"
#include "VideoDecoder.hpp"
#include "WebRTCManager.hpp"

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
void test_authoritative_sync_consistency();
void test_multithreaded_sync_integrity();
void test_sync_detailed_suite();
void test_file_transfer_engine();

void test_webcam_capture() {
    std::cout << "Testing WebcamCapture device enumeration and lifecycle..." << std::endl;
    WebcamCapture cap;
    assert(!cap.IsCapturing());
    auto devices = cap.EnumerateDevices();
    assert(!devices.empty());
    std::cout << "Discovered video capture device: " << devices[0].name << std::endl;
    bool initOk = cap.Initialize(0);
    assert(initOk);
    assert(cap.IsCapturing());
    assert(cap.GetCurrentDeviceInfo().index == 0);
    cap.Update();
    cap.Shutdown();
    assert(!cap.IsCapturing());
    std::cout << "WebcamCapture lifecycle test passed!" << std::endl;
}

void test_video_codec_pipeline() {
    std::cout << "Testing H.264 VideoEncoder and VideoDecoder pipeline..." << std::endl;
    VideoEncoder encoder;
    VideoDecoder decoder;

    VideoEncoderConfig encConfig;
    encConfig.width = 1280;
    encConfig.height = 720;
    encConfig.fps = 30;
    encConfig.bitrate = 2500000;
    assert(encoder.Initialize(nullptr, encConfig));
    assert(encoder.IsInitialized());

    VideoDecoderConfig decConfig;
    decConfig.width = 1280;
    decConfig.height = 720;
    assert(decoder.Initialize(nullptr, decConfig));
    assert(decoder.IsInitialized());

    std::vector<uint8_t> bitstream;
    assert(encoder.EncodeFrame(nullptr, bitstream));
    assert(!bitstream.empty());

    void* decodedTexture = nullptr;
    assert(decoder.DecodeFrame(bitstream, &decodedTexture));
    assert(decodedTexture != nullptr);

    // Test rejection of invalid bitstream
    std::vector<uint8_t> corruptedBitstream = { 0xFF, 0xEE, 0xDD, 0xCC };
    void* corruptedTexture = nullptr;
    assert(!decoder.DecodeFrame(corruptedBitstream, &corruptedTexture));
    assert(corruptedTexture == nullptr);

    encoder.Shutdown();
    decoder.Shutdown();
    assert(!encoder.IsInitialized());
    assert(!decoder.IsInitialized());
    std::cout << "H.264 VideoCodec pipeline passed successfully!" << std::endl;
}

void test_webrtc_manager_pipeline() {
    std::cout << "Testing WebRTCManager pipeline..." << std::endl;
    WebRTCManager rtc;
    assert(!rtc.IsConnected());
    assert(rtc.Initialize(nullptr));

    std::string offerSdp;
    assert(rtc.CreateOffer(offerSdp));
    assert(!offerSdp.empty());
    assert(offerSdp.find("H264") != std::string::npos);

    rtc.AddICECandidate("candidate:1 1 UDP 2130706431 127.0.0.1 50000 typ host");
    assert(rtc.GetICECandidates().size() == 1);

    std::string answerSdp;
    assert(rtc.HandleOffer(offerSdp, answerSdp));
    assert(rtc.IsConnected());

    // Test sending data and frame encoding
    const uint8_t testPayload[] = { 0x01, 0x02, 0x03, 0x04 };
    rtc.SendData(testPayload, sizeof(testPayload));
    assert(rtc.GetQueuedDataBytes() == sizeof(testPayload));

    bool frameCallbackTriggered = false;
    rtc.SetVideoFrameCallback([&frameCallbackTriggered](const std::vector<uint8_t>& frame) {
        if (!frame.empty()) frameCallbackTriggered = true;
    });

    rtc.SendDesktopFrame(nullptr);
    assert(frameCallbackTriggered);

    rtc.Shutdown();
    assert(!rtc.IsConnected());
    assert(rtc.GetQueuedDataBytes() == 0);
    std::cout << "WebRTCManager pipeline passed successfully!" << std::endl;
}

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

void test_authoritative_drift_arbitration() {
    std::cout << "Testing Authoritative Synchronization and Drift Arbitration..." << std::endl;
    SyncModule serverSync;
    SyncModule clientSync;
    unsigned long long clientId = 101;
    unsigned int groupId = 1;

    // Initialize peer position on both sides
    serverSync.UpdatePeer(clientId, groupId, 30000, 30000);
    clientSync.UpdatePeer(clientId, groupId, 30000, 30000);

    PeerState clientState;
    assert(clientSync.GetPeerState(clientId, clientState));
    assert(clientState.normalizedX == 30000);

    // Simulate client-side drift (e.g. packet loss or desync causes client to think it's at 33000)
    int driftedClientX = 33000;
    int driftedClientY = 30000;

    // Server evaluates SyncCheck packet from client
    PeerState serverState;
    assert(serverSync.GetPeerState(clientId, serverState));
    int dx = driftedClientX - serverState.normalizedX;
    int dy = driftedClientY - serverState.normalizedY;
    float drift = std::sqrt((float)(dx * dx + dy * dy));
    assert(drift >= 3000.0f);

    serverSync.UpdateDrift(clientId, drift);
    serverSync.RecordAuthoritativeCorrection(clientId);

    PeerState updatedServerState;
    serverSync.GetPeerState(clientId, updatedServerState);
    assert(updatedServerState.drift >= 3000.0f);
    assert(updatedServerState.authoritativeCorrections == 1);

    // Client receives MasterStateSync with server's authoritative coordinates
    int authoritativeServerX = serverState.normalizedX;
    int authoritativeServerY = serverState.normalizedY;
    clientSync.UpdatePeer(clientId, groupId, authoritativeServerX, authoritativeServerY);
    clientSync.RecordAuthoritativeCorrection(clientId);

    PeerState reconciledClientState;
    assert(clientSync.GetPeerState(clientId, reconciledClientState));
    assert(reconciledClientState.normalizedX == 30000);
    assert(reconciledClientState.authoritativeCorrections == 1);

    std::cout << "Authoritative synchronization test passed!" << std::endl;
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
    test_authoritative_sync_consistency();
    test_multithreaded_sync_integrity();
    test_group_isolation();
    test_sync_detailed_suite();
    test_file_transfer_engine();
    test_webcam_capture();
    test_video_codec_pipeline();
    test_webrtc_manager_pipeline();
    test_authoritative_drift_arbitration();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
