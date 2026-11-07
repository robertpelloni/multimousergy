#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include "webrtc_mock.hpp"
#include "VideoEncoder.hpp"
#include "VideoDecoder.hpp"

#ifdef _WIN32
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
#endif

class WebRTCManager {
public:
    WebRTCManager();
    ~WebRTCManager();

    bool Initialize(void* d3dDevice = nullptr);
    bool CreateOffer(std::string& outSdp);
    bool HandleAnswer(const std::string& sdp);
    bool HandleOffer(const std::string& sdp, std::string& outAnswerSdp);
    void AddICECandidate(const std::string& candidate);

    void SendData(const uint8_t* data, size_t size);
    bool IsConnected() const;
    void SetConnected(bool connected);
    void Shutdown();

    // Media track management
    void SendDesktopFrame(void* frameData);
    void AddVideoTrack();
    void AddAudioTrack();
    void EnableAudioTrack(bool enable);
    bool IsAudioTrackEnabled() const { return m_audioEnabled; }
    void PushAudioFrame(const uint8_t* audioData, size_t size);

    void SetSignalingCallback(std::function<void(const std::string&)> callback) {
        m_signalingCallback = callback;
    }

    // Set callback for receiving incoming video bitstream packets
    void SetVideoFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback) {
        m_videoFrameCallback = callback;
    }

    // Set callback for receiving incoming audio stream packets
    void SetAudioFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback) {
        m_audioFrameCallback = callback;
    }

    // Singleton accessor for subsystems that need the WebRTC pipeline
    static WebRTCManager* GetInstance() { return s_instance; }

    // Desktop frame compression and streaming pipeline
    void EncodeLocalDesktopFrame(void* frameTexture);
    bool DecodeRemoteVideoPacket(const std::vector<uint8_t>& bitstream, void** outTexture);

#ifdef _WIN32
    ID3D11ShaderResourceView* GetRemoteDesktopTexture(unsigned long long peerId);
#endif

    size_t GetQueuedDataBytes() const { return m_queuedBytes; }
    const std::vector<std::string>& GetICECandidates() const { return m_iceCandidates; }

private:
    static WebRTCManager* s_instance;
    bool m_connected;
    bool m_audioEnabled = false;
    size_t m_queuedBytes;
    std::vector<std::string> m_iceCandidates;

    VideoEncoder m_videoEncoder;
    VideoDecoder m_videoDecoder;

    std::function<void(const std::string&)> m_signalingCallback;
    std::function<void(const std::vector<uint8_t>&)> m_videoFrameCallback;
    std::function<void(const std::vector<uint8_t>&)> m_audioFrameCallback;
};
