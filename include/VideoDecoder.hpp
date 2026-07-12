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

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

#ifdef _WIN32
    bool Initialize(ID3D11Device* device);
    bool DecodeFrame(const std::vector<uint8_t>& bitstream, ID3D11Texture2D** outTexture);
#else
    bool Initialize(void* device) { return false; }
    bool DecodeFrame(const std::vector<uint8_t>& bitstream, void** outTexture) { return false; }
#endif

private:
#ifdef _WIN32
    ID3D11Device* m_device;
    IMFTransform* m_decoder;
#endif
};
