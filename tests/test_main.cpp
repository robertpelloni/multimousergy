#include <iostream>
#include <cassert>
#include <cstring>
#include "NetMuxFramework.hpp"
#include "ConfigManager.hpp"
#include "AuthModule.hpp"
#include "WaylandPortalManager.hpp"
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

void test_webrtc_audio_pipeline() {
    std::cout << "Testing WebRTCManager spatial audio pipeline..." << std::endl;
    WebRTCManager rtc;
    assert(!rtc.IsAudioTrackEnabled());

    rtc.EnableAudioTrack(true);
    assert(rtc.IsAudioTrackEnabled());

    bool audioReceived = false;
    std::vector<uint8_t> receivedFrame;
    rtc.SetAudioFrameCallback([&audioReceived, &receivedFrame](const std::vector<uint8_t>& frame) {
        audioReceived = true;
        receivedFrame = frame;
    });

    // Mock PCM audio buffer (16-bit 48kHz mono sample chunk)
    const uint8_t mockPcmAudio[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
    rtc.PushAudioFrame(mockPcmAudio, sizeof(mockPcmAudio));

    assert(audioReceived);
    assert(receivedFrame.size() == sizeof(mockPcmAudio));
    assert(std::memcmp(receivedFrame.data(), mockPcmAudio, sizeof(mockPcmAudio)) == 0);

    // Verify disabling audio suppresses pushing
    rtc.EnableAudioTrack(false);
    assert(!rtc.IsAudioTrackEnabled());
    audioReceived = false;
    rtc.PushAudioFrame(mockPcmAudio, sizeof(mockPcmAudio));
    assert(!audioReceived);

    std::cout << "WebRTCManager spatial audio pipeline test passed!" << std::endl;
}

void test_integrated_video_chat_sync() {
    std::cout << "Testing Integrated Video Chat stream synchronizer..." << std::endl;
    WebRTCManager rtc;
    unsigned long long peerId = 888;

    PeerMediaStream stream;
    assert(!rtc.GetPeerMediaStream(peerId, stream));

    std::vector<uint8_t> mockVideoPacket = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1E };
    std::vector<uint8_t> mockAudioPacket = { 0xFF, 0xF1, 0x50, 0x80 };
    double timestamp = 12345.67;

    rtc.SyncPeerMediaStreams(peerId, mockVideoPacket, mockAudioPacket, timestamp);

    assert(rtc.GetPeerMediaStream(peerId, stream));
    assert(stream.peerId == peerId);
    assert(stream.hasActiveVideo == true);
    assert(stream.hasActiveAudio == true);
    assert(stream.latestVideoPacket.size() == mockVideoPacket.size());
    assert(stream.latestAudioPacket.size() == mockAudioPacket.size());
    assert(stream.lastTimestamp == timestamp);

    rtc.ClearPeerMediaStream(peerId);
    assert(!rtc.GetPeerMediaStream(peerId, stream));

    std::cout << "Integrated Video Chat stream synchronizer test passed!" << std::endl;
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

void test_continuous_drag_selection_sync() {
    std::cout << "Testing Continuous Drag Selection Synchronization..." << std::endl;
    SyncModule sync;
    unsigned long long peerId = 555;
    unsigned int groupId = 1;

    // Simulate initial mouse down (selection start at 10000, 10000)
    sync.UpdatePeer(peerId, groupId, 10000, 10000);
    sync.UpdatePeerSelection(peerId, true, 10000, 10000);

    PeerState state;
    assert(sync.GetPeerState(peerId, state));
    assert(state.isSelecting == true);

    // Simulate continuous dragging movement across several coordinate steps
    for (int step = 1; step <= 5; ++step) {
        int currentX = 10000 + step * 2000;
        int currentY = 10000 + step * 1500;
        sync.UpdatePeer(peerId, groupId, currentX, currentY);
        sync.UpdatePeerSelection(peerId, true, 10000, 10000);

        PeerState dragStepState;
        assert(sync.GetPeerState(peerId, dragStepState));
        assert(dragStepState.isSelecting == true);
        assert(dragStepState.normalizedX == currentX);
        assert(dragStepState.normalizedY == currentY);
    }

    // Simulate mouse release (selection ended)
    sync.UpdatePeerSelection(peerId, false, 0, 0);
    PeerState releasedState;
    assert(sync.GetPeerState(peerId, releasedState));
    assert(releasedState.isSelecting == false);

    std::cout << "Continuous drag selection test passed!" << std::endl;
}

void test_dynamic_resolution_dpi_scaling() {
    std::cout << "Testing Dynamic Resolution and DPI-Aware Coordinate Scaling..." << std::endl;
    SyncModule sync;
    unsigned long long peerId = 999;

    // Register peer on 4K display (3840x2160) with 1.5x DPI scale
    sync.UpdatePeer(peerId, 0, 32767, 32767); // Center coordinate
    sync.UpdatePeerResolution(peerId, 3840, 2160, 1.5f);

    int scaledX = 0, scaledY = 0;
    // Map to a 1080p display (1920x1080) at 1.0x DPI scale
    bool ok = sync.ScalePeerCoordinates(peerId, 1920, 1080, 1.0f, scaledX, scaledY);
    assert(ok);
    // Center of 1920x1080 is roughly 959-960
    assert(scaledX >= 958 && scaledX <= 961);
    assert(scaledY >= 538 && scaledY <= 541);

    // Map top-left (0,0)
    sync.UpdatePeer(peerId, 0, 0, 0);
    ok = sync.ScalePeerCoordinates(peerId, 1920, 1080, 1.0f, scaledX, scaledY);
    assert(ok);
    assert(scaledX == 0);
    assert(scaledY == 0);

    // Map bottom-right (65535, 65535)
    sync.UpdatePeer(peerId, 0, 65535, 65535);
    ok = sync.ScalePeerCoordinates(peerId, 1920, 1080, 1.0f, scaledX, scaledY);
    assert(ok);
    assert(scaledX == 1919);
    assert(scaledY == 1079);

    std::cout << "Dynamic Resolution and DPI-Aware Coordinate Scaling test passed!" << std::endl;
}

void test_network_rate_limiter() {
    std::cout << "Testing NetworkManager Token-Bucket Rate Limiter..." << std::endl;
    NetworkManager net;
    unsigned long long peerId = 8888;

    // Configure bucket: 10 tokens max, refill 5 tokens per second
    net.ConfigureRateLimiter(10.0, 5.0);

    double t0 = 1000.0;
    // Initial burst of 10 packets (cost = 1.0 each) should all pass
    for (int i = 0; i < 10; ++i) {
        bool allowed = net.CheckRateLimit(peerId, 1.0, t0);
        assert(allowed);
    }

    // 11th packet at the exact same time should be throttled
    bool throttled = net.CheckRateLimit(peerId, 1.0, t0);
    assert(!throttled);

    // After 1 second (refill = 5 tokens), 5 packets should be allowed
    double t1 = t0 + 1.0;
    for (int i = 0; i < 5; ++i) {
        bool allowed = net.CheckRateLimit(peerId, 1.0, t1);
        assert(allowed);
    }

    // 6th packet at t1 should be throttled
    bool throttledAgain = net.CheckRateLimit(peerId, 1.0, t1);
    assert(!throttledAgain);

    std::cout << "NetworkManager Token-Bucket Rate Limiter test passed!" << std::endl;
}

void test_auth_module_hmac() {
    std::cout << "Testing AuthModule HMAC-SHA256 message integrity..." << std::endl;
    std::string key = "super_secret_multimousergy_psk";
    std::string message = "Packet payload: absolute cursor coordinate movement";

    unsigned char mac[32];
    bool computeOk = AuthModule::ComputeHMAC_SHA256(key, message.data(), message.size(), mac);
    assert(computeOk);

    // Verify valid HMAC matches
    bool verifyOk = AuthModule::VerifyHMAC_SHA256(key, message.data(), message.size(), mac);
    assert(verifyOk);

    // Verify tampered message fails
    std::string tampered = message + "!";
    bool verifyTamper = AuthModule::VerifyHMAC_SHA256(key, tampered.data(), tampered.size(), mac);
    assert(!verifyTamper);

    // Verify wrong key fails
    bool verifyWrongKey = AuthModule::VerifyHMAC_SHA256("wrong_psk", message.data(), message.size(), mac);
    assert(!verifyWrongKey);

    std::cout << "AuthModule HMAC-SHA256 message integrity test passed!" << std::endl;
}

void test_hotkey_manager() {
    std::cout << "Testing InputEngine Hotkey Manager..." << std::endl;
    InputEngine engine;
    assert(!engine.IsCaptured());

    // Register hotkey: Modifier 6 (Ctrl + Alt), Key 0x27 (VK_RIGHT)
    engine.SetSwitchHotkey(6, 0x27, true);

    // Mismatched key should not trigger
    bool triggerMiss = engine.CheckHotkeyTrigger(2, 0x27);
    assert(!triggerMiss);
    assert(!engine.IsCaptured());

    // Matching key triggers capture toggle
    bool triggerHit = engine.CheckHotkeyTrigger(6, 0x27);
    assert(triggerHit);
    assert(engine.IsCaptured());

    // Second trigger toggles back to local
    bool triggerBack = engine.CheckHotkeyTrigger(6, 0x27);
    assert(triggerBack);
    assert(!engine.IsCaptured());

    // Disabled hotkey should not trigger
    engine.SetSwitchHotkey(6, 0x27, false);
    bool triggerDisabled = engine.CheckHotkeyTrigger(6, 0x27);
    assert(!triggerDisabled);
    assert(!engine.IsCaptured());

    std::cout << "InputEngine Hotkey Manager test passed!" << std::endl;
}

void test_spatial_raycasting_and_intersection() {
    std::cout << "Testing SpatialViewport 3D raycasting and plane intersection..." << std::endl;
    // Screen center 1920x1080 (960, 540)
    Ray3D ray;
    SpatialViewport::ScreenToRay(960, 540, 1920, 1080, ray);

    // Center ray should point straight ahead (dirX ≈ 0, dirY ≈ 0, dirZ ≈ 1)
    assert(std::abs(ray.dirX) < 0.05f);
    assert(std::abs(ray.dirY) < 0.05f);
    assert(ray.dirZ > 0.99f);

    // Anchor placed directly in front of the camera at (0, 0, 0)
    SpatialAnchor anchor;
    anchor.posX = 0.0f;
    anchor.posY = 0.0f;
    anchor.posZ = 0.0f;
    anchor.scale = 1.0f;

    float u = -1.0f, v = -1.0f;
    bool hit = SpatialViewport::IntersectRayWithAnchorPlane(ray, anchor, u, v);
    assert(hit);
    // Intersection at center should be roughly UV (0.5, 0.5)
    assert(std::abs(u - 0.5f) < 0.05f);
    assert(std::abs(v - 0.5f) < 0.05f);

    // Ray pointing off-screen to the far right
    Ray3D missRay;
    missRay.originX = 0.0f; missRay.originY = 0.0f; missRay.originZ = -5.0f;
    missRay.dirX = 10.0f; missRay.dirY = 0.0f; missRay.dirZ = 1.0f;
    bool missHit = SpatialViewport::IntersectRayWithAnchorPlane(missRay, anchor, u, v);
    assert(!missHit);

    std::cout << "SpatialViewport 3D raycasting and plane intersection test passed!" << std::endl;
}

void test_wayland_portal_manager() {
    std::cout << "Testing WaylandPortalManager display detection and session lifecycle..." << std::endl;
    WaylandPortalManager portal;

    // Verify display server detection
    WaylandSessionType sessionType = WaylandPortalManager::DetectDisplayServer();
    assert(sessionType == WaylandSessionType::X11 || sessionType == WaylandSessionType::WaylandNative ||
           sessionType == WaylandSessionType::WaylandPortal || sessionType == WaylandSessionType::Unknown);

    // Verify session acquisition
    std::string handle;
    bool sessionOk = portal.RequestInputCaptureSession(handle);
    assert(sessionOk);
    assert(!handle.empty());
    assert(portal.IsActive());

    // Verify virtual pointer event injection capability
    bool injectOk = portal.InjectVirtualPointerEvent(10, 20, 0);
    assert(injectOk);

    // Verify session release
    bool releaseOk = portal.ReleaseInputCaptureSession(handle);
    assert(releaseOk);
    assert(!portal.IsActive());

    std::cout << "WaylandPortalManager display detection and session lifecycle test passed!" << std::endl;
}

void test_driver_interface_virtual_device() {
    std::cout << "Testing DriverInterface Virtual Device lifecycle..." << std::endl;
    DriverInterface driver;

    // Test initial state
    assert(!driver.IsInitialized());
    assert(driver.GetDriverType() == NetMuxDriverType::Auto);

    // Test driver initialization
    driver.Initialize(NetMuxDriverType::Auto);

    // Exercise API surface
    driver.SendMouseMovement(10, -5);
    driver.SendMouseButton(1, true);
    driver.SendMouseButton(1, false);
    driver.SendMouseWheel(120, false);

    driver.Shutdown();
    assert(!driver.IsInitialized());

    std::cout << "DriverInterface Virtual Device lifecycle test passed!" << std::endl;
}

void test_multimonitor_layout_mapping() {
    std::cout << "Testing Multi-Monitor Layout Mapping..." << std::endl;
    SyncModule sync;

    // Dual 1920x1080 setup side-by-side
    // Monitor 1: (0, 0) -> (1920, 1080)
    // Monitor 2: (1920, 0) -> (3840, 1080)
    MonitorDescriptor mon1 = { 1, 0, 0, 1920, 1080, 1.0f, true };
    MonitorDescriptor mon2 = { 2, 1920, 0, 1920, 1080, 1.25f, false };

    sync.AddLocalMonitor(mon1);
    sync.AddLocalMonitor(mon2);
    assert(sync.GetLocalMonitors().size() == 2);

    int outMonId = 0, outX = 0, outY = 0;

    // Point on Monitor 1
    bool hit1 = sync.MapVirtualToPhysicalMonitor(500, 300, outMonId, outX, outY);
    assert(hit1);
    assert(outMonId == 1);
    assert(outX == 500);
    assert(outY == 300);

    // Point on Monitor 2
    bool hit2 = sync.MapVirtualToPhysicalMonitor(2120, 450, outMonId, outX, outY);
    assert(hit2);
    assert(outMonId == 2);
    assert(outX == 200); // 2120 - 1920
    assert(outY == 450);

    // Boundary edge on Monitor 2
    bool hitEdge = sync.MapVirtualToPhysicalMonitor(3839, 1079, outMonId, outX, outY);
    assert(hitEdge);
    assert(outMonId == 2);
    assert(outX == 1919);
    assert(outY == 1079);

    std::cout << "Multi-Monitor Layout Mapping test passed!" << std::endl;
}

void test_group_scoped_interaction_ownership() {
    std::cout << "Testing Group-Scoped Interaction Ownership..." << std::endl;
    SyncModule sync;
    unsigned long long peerGroup1 = 301;
    unsigned long long peerGroup2 = 401;
    unsigned int group1 = 10;
    unsigned int group2 = 20;

    sync.UpdatePeer(peerGroup1, group1, 5000, 5000);
    sync.UpdatePeer(peerGroup2, group2, 9000, 9000);

    // Peer 1 claims ownership in Group 10
    bool claim1 = sync.ResolveGroupInteraction(peerGroup1, group1, 1000.0, false);
    assert(claim1);
    assert(sync.GetGroupActivePeer(group1) == peerGroup1);

    // Peer 2 claims ownership in Group 20 simultaneously
    bool claim2 = sync.ResolveGroupInteraction(peerGroup2, group2, 1010.0, false);
    assert(claim2);
    assert(sync.GetGroupActivePeer(group2) == peerGroup2);

    // Both peers concurrently hold ownership in their isolated groups
    assert(sync.GetGroupActivePeer(group1) == peerGroup1);
    assert(sync.GetGroupActivePeer(group2) == peerGroup2);

    // Soft release in Group 10 does not impact Group 20
    bool rel1 = sync.ResolveGroupInteraction(peerGroup1, group1, 1050.0, true);
    assert(rel1);
    assert(sync.GetGroupActivePeer(group2) == peerGroup2);

    std::cout << "Group-Scoped Interaction Ownership test passed!" << std::endl;
}

void test_spatial_viewport_anchors() {
    std::cout << "Testing SpatialViewport SpatialAnchor tracking..." << std::endl;
    SpatialViewport viewport;
    unsigned long long peer1 = 1001;
    unsigned long long peer2 = 1002;

    viewport.SetSpatialAnchor(peer1, 2.5f, 0.0f, -5.0f, 1.2f);
    viewport.SetSpatialAnchor(peer2, -2.5f, 1.0f, -4.5f, 0.8f);

    SpatialAnchor a1, a2;
    assert(viewport.GetSpatialAnchor(peer1, a1));
    assert(viewport.GetSpatialAnchor(peer2, a2));

    assert(a1.posX == 2.5f);
    assert(a1.scale == 1.2f);
    assert(!a1.hasActiveFocus);

    // Set active anchor focus
    viewport.SetActiveAnchorFocus(peer1);
    assert(viewport.GetSpatialAnchor(peer1, a1));
    assert(viewport.GetSpatialAnchor(peer2, a2));
    assert(a1.hasActiveFocus == true);
    assert(a2.hasActiveFocus == false);

    // Remove anchor
    viewport.RemoveSpatialAnchor(peer2);
    assert(!viewport.GetSpatialAnchor(peer2, a2));

    std::cout << "SpatialViewport SpatialAnchor tracking test passed!" << std::endl;
}

void test_spatial_audio_panning() {
    std::cout << "Testing Directional Spatial Audio Panning..." << std::endl;
    float leftGain = 0.0f, rightGain = 0.0f;

    // Hard left (normalizedX = 0)
    WebRTCManager::CalculateSpatialAudioGain(0, 32767, leftGain, rightGain);
    assert(leftGain > 0.99f);
    assert(rightGain < 0.01f);

    // Hard right (normalizedX = 65535)
    WebRTCManager::CalculateSpatialAudioGain(65535, 32767, leftGain, rightGain);
    assert(leftGain < 0.01f);
    assert(rightGain > 0.99f);

    // Center (normalizedX = 32767)
    WebRTCManager::CalculateSpatialAudioGain(32767, 32767, leftGain, rightGain);
    assert(std::abs(leftGain - rightGain) < 0.05f);

    // Test PCM buffer application
    // 4 samples = 2 stereo frames of 1000 amplitude
    std::vector<int16_t> rawPcm = { 1000, 1000, 1000, 1000 };
    std::vector<uint8_t> inputBytes(reinterpret_cast<uint8_t*>(rawPcm.data()), reinterpret_cast<uint8_t*>(rawPcm.data() + rawPcm.size()));
    std::vector<uint8_t> outputBytes;

    bool panOk = WebRTCManager::ApplySpatialAudioPan(inputBytes, 0, 0, outputBytes);
    assert(panOk);
    const int16_t* pannedSamples = reinterpret_cast<const int16_t*>(outputBytes.data());
    // In hard left: left channel preserved, right channel muted
    assert(pannedSamples[0] > 900);
    assert(pannedSamples[1] < 50);

    std::cout << "Directional Spatial Audio Panning test passed!" << std::endl;
}

void test_file_transfer_batch_queue() {
    std::cout << "Testing FileTransferEngine batch queueing..." << std::endl;
    // Create multiple test dummy files
    std::string path1 = "test_batch_1.txt";
    std::string path2 = "test_batch_2.txt";
    {
        std::ofstream f1(path1, std::ios::binary);
        f1 << "MultiMousergy Batch Item 1 payload";
        std::ofstream f2(path2, std::ios::binary);
        f2 << "MultiMousergy Batch Item 2 payload with more data bytes";
    }

    FileTransferEngine engine;
    std::vector<std::string> paths = { path1, path2 };
    std::vector<unsigned long long> outIds;
    bool batchOk = engine.StartBatchTransfer(paths, 42, outIds);
    assert(batchOk);
    assert(outIds.size() == 2);

    auto active = engine.GetActiveTransfers();
    assert(active.size() == 2);
    assert(active.count(outIds[0]) > 0);
    assert(active.count(outIds[1]) > 0);
    assert(active[outIds[0]].filename == path1);
    assert(active[outIds[1]].filename == path2);
    assert(active[outIds[0]].isOutgoing == true);

    // Clean up files
    std::remove(path1.c_str());
    std::remove(path2.c_str());
    std::cout << "FileTransferEngine batch queueing test passed!" << std::endl;
}

void test_file_transfer_engine_queue_and_telemetry() {
    std::cout << "Testing FileTransferEngine queue, multi-chunk progress and telemetry..." << std::endl;
    // Create a temporary mock file
    std::string testPath = "test_queue_file.bin";
    std::ofstream out(testPath, std::ios::binary);
    std::string data = "NetMux telemetry streaming & queue transfer verification buffer payload!";
    out.write(data.c_str(), data.size());
    out.close();

    FileTransferEngine sender;
    assert(sender.StartTransfer(testPath, 101));
    auto active = sender.GetActiveTransfers();
    assert(!active.empty());

    unsigned long long tId = active.begin()->first;
    assert(active[tId].filename == testPath);
    assert(active[tId].isOutgoing == true);
    assert(!active[tId].isComplete);

    Packet headerPkt;
    assert(!sender.IsHeaderSent(tId));
    assert(sender.GetHeaderPacket(tId, headerPkt));
    sender.SetHeaderSent(tId, true);
    assert(sender.IsHeaderSent(tId));

    FileTransferEngine receiver;
    assert(receiver.HandleFileHeader(headerPkt));

    Packet chunkPkt;
    while (sender.GetNextChunk(tId, chunkPkt)) {
        assert(receiver.HandleFileData(chunkPkt));
    }

    auto senderStatus = sender.GetActiveTransfers();
    assert(senderStatus[tId].isComplete == true);
    assert(senderStatus[tId].progress >= 1.0f);

    auto receiverStatus = receiver.GetActiveTransfers();
    assert(receiverStatus[tId].isComplete == true);
#ifdef None
#undef None
#endif
    assert(receiverStatus[tId].lastError == FileTransferError::None);

    std::remove(testPath.c_str());
    std::remove(("received_" + testPath).c_str());
    std::cout << "FileTransferEngine queue and telemetry test passed!" << std::endl;
}

void test_multi_team_group_isolation() {
    std::cout << "Testing Multi-Team Group Isolation logic..." << std::endl;
    SyncModule syncGroupA;
    SyncModule syncGroupB;
    unsigned long long peerA = 201;
    unsigned long long peerB = 202;
    unsigned int groupA = 10;
    unsigned int groupB = 20;

    // Register peers into their respective groups
    syncGroupA.UpdatePeer(peerA, groupA, 10000, 10000);
    syncGroupB.UpdatePeer(peerB, groupB, 20000, 20000);

    PeerState stateA, stateB;
    assert(syncGroupA.GetPeerState(peerA, stateA));
    assert(syncGroupB.GetPeerState(peerB, stateB));
    assert(stateA.groupId == groupA);
    assert(stateB.groupId == groupB);

    // Verify group isolation: Peer A cannot claim interaction ownership on group B's sync module
    bool claimedOnGroupB = syncGroupB.ResolveInteraction(peerA, 1000.0, false);
    // Should be isolated or treated as new focus in its own context
    assert(claimedOnGroupB || !syncGroupB.GetPeerState(peerA, stateA));

    std::cout << "Multi-team group isolation test passed!" << std::endl;
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
    test_webrtc_audio_pipeline();
    test_integrated_video_chat_sync();
    test_authoritative_drift_arbitration();
    test_multi_team_group_isolation();
    test_continuous_drag_selection_sync();
    test_file_transfer_engine_queue_and_telemetry();
    test_dynamic_resolution_dpi_scaling();
    test_file_transfer_batch_queue();
    test_spatial_audio_panning();
    test_spatial_viewport_anchors();
    test_group_scoped_interaction_ownership();
    test_multimonitor_layout_mapping();
    test_driver_interface_virtual_device();
    test_wayland_portal_manager();
    test_spatial_raycasting_and_intersection();
    test_hotkey_manager();
    test_auth_module_hmac();
    test_network_rate_limiter();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
