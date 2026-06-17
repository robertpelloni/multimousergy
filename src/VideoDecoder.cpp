#include "VideoDecoder.hpp"
#include <iostream>

VideoDecoder::VideoDecoder() {
#ifdef _WIN32
    m_device = nullptr;
    m_decoder = nullptr;
#endif
}

VideoDecoder::~VideoDecoder() {
#ifdef _WIN32
    if (m_decoder) m_decoder->Release();
#endif
}

#ifdef _WIN32
#include <wmcodecdsp.h>

bool VideoDecoder::Initialize(ID3D11Device* device) {
    m_device = device;
    std::cout << "[VideoDecoder] Initializing hardware H.264 decoder (Media Foundation MFT)..." << std::endl;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_decoder));
    if (FAILED(hr)) return false;

    return true;
}

bool VideoDecoder::DecodeFrame(const std::vector<uint8_t>& bitstream, ID3D11Texture2D** outTexture) {
    if (!m_device || bitstream.empty()) return false;

    // 1. Create IMFMediaBuffer from bitstream
    // 2. Create IMFSample and add buffer
    // 3. ProcessInput on m_decoder
    // 4. ProcessOutput and retrieve D3D11 texture

    return true;
}
#endif
