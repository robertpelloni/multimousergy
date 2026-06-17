#pragma once
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <d3d11.h>
#endif

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

#ifdef _WIN32
    bool Initialize(ID3D11Device* device);
    bool EncodeFrame(ID3D11Texture2D* texture, std::vector<uint8_t>& outBitstream);
#else
    bool Initialize(void* device) { return false; }
    bool EncodeFrame(void* texture, std::vector<uint8_t>& outBitstream) { return false; }
#endif

private:
#ifdef _WIN32
    ID3D11Device* m_device;
#endif
};
