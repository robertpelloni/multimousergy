#include "VideoDecoder.hpp"
#include <iostream>
#include <cstring>

VideoDecoder::VideoDecoder()
    : m_initialized(false) {
#ifdef _WIN32
    m_device = nullptr;
    m_decoder = nullptr;
    m_inputType = nullptr;
    m_outputType = nullptr;
#endif
}

VideoDecoder::~VideoDecoder() {
    Shutdown();
}

void VideoDecoder::Shutdown() {
#ifdef _WIN32
    if (m_outputType) {
        m_outputType->Release();
        m_outputType = nullptr;
    }
    if (m_inputType) {
        m_inputType->Release();
        m_inputType = nullptr;
    }
    if (m_decoder) {
        m_decoder->Release();
        m_decoder = nullptr;
    }
    m_device = nullptr;
#endif
    m_initialized = false;
}

#ifdef _WIN32
#include <wmcodecdsp.h>

bool VideoDecoder::Initialize(void* devicePtr, const VideoDecoderConfig& config) {
    Shutdown();
    m_config = config;
    m_device = static_cast<ID3D11Device*>(devicePtr);

    std::cout << "[VideoDecoder] Initializing hardware H.264 decoder (Media Foundation MFT) "
              << m_config.width << "x" << m_config.height << "..." << std::endl;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_decoder));
    if (FAILED(hr) || !m_decoder) return false;

    // Set input media type (H.264)
    hr = MFCreateMediaType(&m_inputType);
    if (FAILED(hr)) return false;
    m_inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    m_inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    MFSetAttributeSize(m_inputType, MF_MT_FRAME_SIZE, m_config.width, m_config.height);
    hr = m_decoder->SetInputType(0, m_inputType, 0);
    if (FAILED(hr)) return false;

    // Set output media type (NV12 / RGB32)
    hr = MFCreateMediaType(&m_outputType);
    if (FAILED(hr)) return false;
    m_outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    m_outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(m_outputType, MF_MT_FRAME_SIZE, m_config.width, m_config.height);
    hr = m_decoder->SetOutputType(0, m_outputType, 0);
    if (FAILED(hr)) return false;

    m_initialized = true;
    return true;
}

bool VideoDecoder::DecodeFrame(const std::vector<uint8_t>& bitstream, void** outTexturePtr) {
    if (!outTexturePtr) return false;
    *outTexturePtr = nullptr;
    if (!m_initialized || bitstream.empty() || !m_decoder) return false;

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
                            ID3D11Texture2D* d3dTex = nullptr;
                            pDXGIBuf->GetResource(IID_PPV_ARGS(&d3dTex));
                            *outTexturePtr = static_cast<void*>(d3dTex);
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

    return (*outTexturePtr) != nullptr;
}
#else
// Cross-platform mock implementation for CI and non-Windows targets
bool VideoDecoder::Initialize(void* devicePtr, const VideoDecoderConfig& config) {
    m_config = config;
    m_initialized = true;
    std::cout << "[VideoDecoder] (Cross-Platform) Initialized video decoder simulator: "
              << m_config.width << "x" << m_config.height << std::endl;
    return true;
}

bool VideoDecoder::DecodeFrame(const std::vector<uint8_t>& bitstream, void** outTexturePtr) {
    if (!outTexturePtr) return false;
    *outTexturePtr = nullptr;
    if (!m_initialized || bitstream.empty()) return false;

    // Verify H.264 Annex B start code prefix (0x00, 0x00, 0x00, 0x01 or 0x00, 0x00, 0x01)
    bool hasStartCode = false;
    if (bitstream.size() >= 4 && bitstream[0] == 0x00 && bitstream[1] == 0x00 && bitstream[2] == 0x00 && bitstream[3] == 0x01) {
        hasStartCode = true;
    } else if (bitstream.size() >= 3 && bitstream[0] == 0x00 && bitstream[1] == 0x00 && bitstream[2] == 0x01) {
        hasStartCode = true;
    }

    if (!hasStartCode) {
        std::cerr << "[VideoDecoder] Invalid H.264 stream: missing start code prefix." << std::endl;
        return false;
    }

    // Allocate dummy texture handle representing decoded DXGI frame
    static uint32_t s_mockFrameHandle = 0xDECAD001;
    *outTexturePtr = reinterpret_cast<void*>(&s_mockFrameHandle);
    return true;
}
#endif
