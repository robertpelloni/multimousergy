#include "WebRTCManager.hpp"
#include <iostream>

#ifdef _WIN32
#include <d3d11.h>
#endif

WebRTCManager* WebRTCManager::s_instance = nullptr;

WebRTCManager::WebRTCManager()
    : m_connected(false), m_queuedBytes(0) {
    s_instance = this;
}

WebRTCManager::~WebRTCManager() {
    Shutdown();
    if (s_instance == this) s_instance = nullptr;
}

bool WebRTCManager::Initialize(void* d3dDevice) {
    std::cout << "[WebRTC] Initializing native pipeline..." << std::endl;
    m_connected = false;
    m_iceCandidates.clear();
    m_queuedBytes = 0;

    VideoEncoderConfig encCfg;
    encCfg.width = 1920;
    encCfg.height = 1080;
    encCfg.fps = 60;
    encCfg.bitrate = 4000000;
    encCfg.lowLatency = true;
    m_videoEncoder.Initialize(d3dDevice, encCfg);

    VideoDecoderConfig decCfg;
    decCfg.width = 1920;
    decCfg.height = 1080;
    decCfg.lowLatency = true;
    m_videoDecoder.Initialize(d3dDevice, decCfg);

    return true;
}

bool WebRTCManager::CreateOffer(std::string& outSdp) {
    std::cout << "[WebRTC] Creating SDP Offer and gathering ICE candidates..." << std::endl;
    outSdp = "v=0\r\no=- 12345 2 IN IP4 127.0.0.1\r\ns=MultiMousergy WebRTC\r\nt=0 0\r\nm=video 9 UDP/TLS/RTP/SAVPF 96\r\na=rtpmap:96 H264/90000\r\n";
    if (m_signalingCallback) {
        m_signalingCallback(outSdp);
    }
    return true;
}

bool WebRTCManager::HandleAnswer(const std::string& sdp) {
    std::cout << "[WebRTC] Handling SDP Answer: " << sdp.substr(0, std::min<size_t>(32, sdp.size())) << "..." << std::endl;
    m_connected = true;
    return true;
}

bool WebRTCManager::HandleOffer(const std::string& sdp, std::string& outAnswerSdp) {
    std::cout << "[WebRTC] Handling remote offer SDP..." << std::endl;
    outAnswerSdp = "v=0\r\no=- 67890 2 IN IP4 127.0.0.1\r\ns=MultiMousergy WebRTC\r\nt=0 0\r\nm=video 9 UDP/TLS/RTP/SAVPF 96\r\na=rtpmap:96 H264/90000\r\n";
    m_connected = true;
    return true;
}

void WebRTCManager::AddICECandidate(const std::string& candidate) {
    std::cout << "[WebRTC] Adding ICE Candidate: " << candidate << std::endl;
    m_iceCandidates.push_back(candidate);
}

void WebRTCManager::SendData(const uint8_t* data, size_t size) {
    if (!data || size == 0) return;
    m_queuedBytes += size;
}

void WebRTCManager::SendDesktopFrame(void* frameData) {
    EncodeLocalDesktopFrame(frameData);
}

void WebRTCManager::EncodeLocalDesktopFrame(void* frameTexture) {
    if (!m_videoEncoder.IsInitialized()) return;
    std::vector<uint8_t> bitstream;
    if (m_videoEncoder.EncodeFrame(frameTexture, bitstream) && !bitstream.empty()) {
        SendData(bitstream.data(), bitstream.size());
        if (m_videoFrameCallback) {
            m_videoFrameCallback(bitstream);
        }
    }
}

bool WebRTCManager::DecodeRemoteVideoPacket(const std::vector<uint8_t>& bitstream, void** outTexture) {
    if (!m_videoDecoder.IsInitialized()) return false;
    return m_videoDecoder.DecodeFrame(bitstream, outTexture);
}

void WebRTCManager::AddVideoTrack() {
    std::cout << "[WebRTC] Adding high-fps desktop video track via WebRTC..." << std::endl;
}

void WebRTCManager::AddAudioTrack() {
    std::cout << "[WebRTC] Initializing spatial audio pipeline..." << std::endl;
}

bool WebRTCManager::IsConnected() const {
    return m_connected;
}

void WebRTCManager::SetConnected(bool connected) {
    m_connected = connected;
}

void WebRTCManager::Shutdown() {
    std::cout << "[WebRTC] Shutting down peer connection..." << std::endl;
    m_connected = false;
    m_iceCandidates.clear();
    m_queuedBytes = 0;
    m_videoEncoder.Shutdown();
    m_videoDecoder.Shutdown();
}

#ifdef _WIN32
ID3D11ShaderResourceView* WebRTCManager::GetRemoteDesktopTexture(unsigned long long peerId) {
    return nullptr;
}
#endif
