#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "webrtc_mock.hpp"

class WebRTCManager {
public:
    WebRTCManager();
    ~WebRTCManager();

    bool Initialize();
    bool CreateOffer();
    bool HandleAnswer(const std::string& sdp);
    bool HandleOffer(const std::string& sdp);

    void SendData(const uint8_t* data, size_t size);

    // Media track management
    void SendDesktopFrame(void* frameData);
    void AddVideoTrack();
    void AddAudioTrack();

    void SetSignalingCallback(std::function<void(const std::string&)> callback) {
        m_signalingCallback = callback;
    }

private:
    rtc::Thread* m_networkThread;
    rtc::Thread* m_workerThread;
    rtc::Thread* m_signalingThread;
    webrtc::PeerConnectionFactoryInterface* m_factory;
    webrtc::PeerConnectionInterface* m_peerConnection;
    webrtc::DataChannelInterface* m_dataChannel;

    std::function<void(const std::string&)> m_signalingCallback;
};
