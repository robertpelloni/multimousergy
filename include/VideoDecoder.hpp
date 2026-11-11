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

struct VideoDecoderConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool lowLatency = true;
};

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    bool Initialize(void* device, const VideoDecoderConfig& config = VideoDecoderConfig());
    bool DecodeFrame(const std::vector<uint8_t>& bitstream, void** outTexture);
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }
    VideoDecoderConfig GetConfig() const { return m_config; }

private:
    bool m_initialized;
    VideoDecoderConfig m_config;

#ifdef _WIN32
    ID3D11Device* m_device;
    IMFTransform* m_decoder;
    IMFMediaType* m_inputType;
    IMFMediaType* m_outputType;
#endif
};
