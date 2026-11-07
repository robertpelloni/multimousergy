#pragma once
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <d3d11.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mftransform.h>
#endif

struct VideoEncoderConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    uint32_t bitrate = 5000000; // 5 Mbps
    bool lowLatency = true;
};

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    bool Initialize(void* device, const VideoEncoderConfig& config = VideoEncoderConfig());
    bool EncodeFrame(void* texture, std::vector<uint8_t>& outBitstream);
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }
    VideoEncoderConfig GetConfig() const { return m_config; }

private:
    bool m_initialized;
    VideoEncoderConfig m_config;

#ifdef _WIN32
    ID3D11Device* m_device;
    IMFTransform* m_encoder;
    IMFMediaType* m_inputType;
    IMFMediaType* m_outputType;
    DWORD m_inputBufferId;
    DWORD m_outputBufferId;
#endif
};
