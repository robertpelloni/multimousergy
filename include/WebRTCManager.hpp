#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "webrtc_mock.hpp"

#ifdef _WIN32
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
#endif

class WebRTCManager {
public:
    WebRTCManager();
    ~WebRTCManager();

    bool Initialize();
    bool CreateOffer(std::string& outSdp);
    bool HandleAnswer(const std::string& sdp);
    bool HandleOffer(const std::string& sdp, std::string& outAnswerSdp);
    void AddICECandidate(const std::string& candidate);

    void SendData(const uint8_t* data, size_t size);
    bool IsConnected() const;
    void Shutdown();

    // Media track management
    void SendDesktopFrame(void* frameData);
    void AddVideoTrack();
    void AddAudioTrack();

    void SetSignalingCallback(std::function<void(const std::string&)> callback) {
        m_signalingCallback = callback;
    }

    // Singleton accessor for subsystems that need the WebRTC pipeline
    static WebRTCManager* GetInstance() { return s_instance; }

    // Remote desktop texture streaming (DXGI path)
#ifdef _WIN32
    ID3D11ShaderResourceView* GetRemoteDesktopTexture(unsigned long long peerId);
    void EncodeLocalDesktopFrame(ID3D11Texture2D* frame);
#endif

private:
    static WebRTCManager* s_instance;
    rtc::Thread* m_networkThread;
    rtc::Thread* m_workerThread;
    rtc::Thread* m_signalingThread;
    webrtc::PeerConnectionFactoryInterface* m_factory;
    webrtc::PeerConnectionInterface* m_peerConnection;
    webrtc::DataChannelInterface* m_dataChannel;

    std::function<void(const std::string&)> m_signalingCallback;
};
