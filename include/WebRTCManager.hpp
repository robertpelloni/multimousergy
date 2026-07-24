#pragma once
#include <string>
#include <memory>
#include <vector>

// Forward declarations for WebRTC types
namespace webrtc {
    class PeerConnectionInterface;
    class DataChannelInterface;
    class PeerConnectionFactoryInterface;
    class VideoTrackInterface;
    class AudioTrackInterface;
}

namespace rtc {
    class Thread;
}

class WebRTCManager {
public:
    WebRTCManager();
    ~WebRTCManager();

    bool Initialize();
    bool CreateOffer();
    bool HandleAnswer(const std::string& sdp);

    void SendData(const uint8_t* data, size_t size);

    // Media track management
    void AddVideoTrack();
    void AddAudioTrack();

private:
    // rtc::Thread* m_networkThread;
    // rtc::Thread* m_workerThread;
    // rtc::Thread* m_signalingThread;
    // webrtc::PeerConnectionFactoryInterface* m_factory;
    // webrtc::PeerConnectionInterface* m_peerConnection;
    // webrtc::DataChannelInterface* m_dataChannel;
};
