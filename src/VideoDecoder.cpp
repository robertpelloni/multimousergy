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
    if (!m_device || bitstream.empty() || !m_decoder) return false;

    IMFMediaBuffer* pBuffer = nullptr;
    HRESULT hr = MFCreateMemoryBuffer((DWORD)bitstream.size(), &pBuffer);
    if (FAILED(hr)) return false;

    BYTE* pData = nullptr;
    hr = pBuffer->Lock(&pData, NULL, NULL);
    if (SUCCEEDED(hr)) {
        memcpy(pData, bitstream.data(), bitstream.size());
        pBuffer->SetCurrentLength((DWORD)bitstream.size());
        pBuffer->Unlock();

        IMFSample* pSample = nullptr;
        hr = MFCreateSample(&pSample);
        if (SUCCEEDED(hr)) {
            pSample->AddBuffer(pBuffer);
            hr = m_decoder->ProcessInput(0, pSample, 0);

            if (SUCCEEDED(hr)) {
                MFT_OUTPUT_DATA_BUFFER outputData = {0};
                DWORD status = 0;
                hr = m_decoder->ProcessOutput(0, 1, &outputData, &status);

                if (hr == S_OK && outputData.pSample) {
                    IMFMediaBuffer* pOutBuf = nullptr;
                    outputData.pSample->GetBufferByIndex(0, &pOutBuf);
                    if (pOutBuf) {
                        IMFDXGIBuffer* pDXGIBuf = nullptr;
                        hr = pOutBuf->QueryInterface(IID_PPV_ARGS(&pDXGIBuf));
                        if (SUCCEEDED(hr)) {
                            pDXGIBuf->GetResource(IID_PPV_ARGS(outTexture));
                            pDXGIBuf->Release();
                        }
                        pOutBuf->Release();
                    }
                    outputData.pSample->Release();
                }
                if (outputData.pEvents) outputData.pEvents->Release();
            }
            pSample->Release();
        }
    }
    pBuffer->Release();

    return (*outTexture) != nullptr;
}
#endif
