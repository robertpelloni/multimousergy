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
    IMFTransform* m_encoder;
    IMFMediaType* m_inputType;
    IMFMediaType* m_outputType;
    DWORD m_inputBufferId;
    DWORD m_outputBufferId;
#endif
};
