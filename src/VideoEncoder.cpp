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
    std::cout << "[VideoEncoder] EXPERIMENTAL: Initializing hardware H.264 encoder (Media Foundation MFT)..." << std::endl;

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
    if (!m_device || !texture || !m_encoder) return false;

    IMFSample* pSample = nullptr;
    IMFMediaBuffer* pBuffer = nullptr;
    HRESULT hr = MFCreateSample(&pSample);
    if (FAILED(hr)) return false;

    // Direct D3D11 surface wrapping for MFT
    hr = MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), texture, 0, FALSE, &pBuffer);
    if (SUCCEEDED(hr)) {
        pSample->AddBuffer(pBuffer);

        hr = m_encoder->ProcessInput(0, pSample, 0);
        if (SUCCEEDED(hr)) {
            MFT_OUTPUT_DATA_BUFFER outputData = {0};
            outputData.dwStreamID = 0;
            DWORD status = 0;
            hr = m_encoder->ProcessOutput(0, 1, &outputData, &status);

            if (hr == S_OK && outputData.pSample) {
                IMFMediaBuffer* pOutBuf = nullptr;
                outputData.pSample->GetBufferByIndex(0, &pOutBuf);
                if (pOutBuf) {
                    BYTE* pData = nullptr;
                    DWORD len = 0;
                    pOutBuf->Lock(&pData, NULL, &len);
                    if (pData && len > 0) {
                        outBitstream.insert(outBitstream.end(), pData, pData + len);
                    }
                    pOutBuf->Unlock();
                    pOutBuf->Release();
                }
                outputData.pSample->Release();
            }
            if (outputData.pEvents) outputData.pEvents->Release();
        }
        pBuffer->Release();
    }
    pSample->Release();

    return !outBitstream.empty();
}
#endif
