#include "VideoEncoder.hpp"
#include <iostream>

VideoEncoder::VideoEncoder() {
#ifdef _WIN32
    m_device = nullptr;
    m_encoder = nullptr;
    m_inputType = nullptr;
    m_outputType = nullptr;
#endif
}

VideoEncoder::~VideoEncoder() {
#ifdef _WIN32
    if (m_encoder) m_encoder->Release();
    if (m_inputType) m_inputType->Release();
    if (m_outputType) m_outputType->Release();
#endif
}

#ifdef _WIN32
#include <wmcodecdsp.h>
#include <codecapi.h>

bool VideoEncoder::Initialize(ID3D11Device* device) {
    m_device = device;
    std::cout << "[VideoEncoder] Initializing hardware H.264 encoder (Media Foundation MFT)..." << std::endl;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_encoder));
    if (FAILED(hr)) return false;

    // Set low-latency mode and CBR
    ICodecAPI* pCodecApi = nullptr;
    hr = m_encoder->QueryInterface(IID_PPV_ARGS(&pCodecApi));
    if (SUCCEEDED(hr)) {
        VARIANT var = {0};
        var.vt = VT_BOOL; var.boolVal = VARIANT_TRUE;
        pCodecApi->SetValue(&CODECAPI_AVLowLatencyMode, &var);
        pCodecApi->Release();
    }

    return true;
}

bool VideoEncoder::EncodeFrame(ID3D11Texture2D* texture, std::vector<uint8_t>& outBitstream) {
    if (!m_device || !texture) return false;

    // 1. Create IMFSample from D3D11 Texture
    // 2. ProcessInput(m_inputBufferId, pSample, 0)
    // 3. ProcessOutput() to retrieve H.264 NAL units
    // 4. Append to outBitstream

    return true;
}
#endif
