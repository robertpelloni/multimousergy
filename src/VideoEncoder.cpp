#include "VideoEncoder.hpp"
#include <iostream>

VideoEncoder::VideoEncoder()
    : m_initialized(false) {
#ifdef _WIN32
    m_device = nullptr;
    m_encoder = nullptr;
    m_inputType = nullptr;
    m_outputType = nullptr;
    m_inputBufferId = 0;
    m_outputBufferId = 0;
#endif
}

VideoEncoder::~VideoEncoder() {
    Shutdown();
}

void VideoEncoder::Shutdown() {
#ifdef _WIN32
    if (m_outputType) {
        m_outputType->Release();
        m_outputType = nullptr;
    }
    if (m_inputType) {
        m_inputType->Release();
        m_inputType = nullptr;
    }
    if (m_encoder) {
        m_encoder->Release();
        m_encoder = nullptr;
    }
    m_device = nullptr;
#endif
    m_initialized = false;
}

#ifdef _WIN32
#include <wmcodecdsp.h>
#include <codecapi.h>

bool VideoEncoder::Initialize(void* devicePtr, const VideoEncoderConfig& config) {
    Shutdown();
    m_config = config;
    m_device = static_cast<ID3D11Device*>(devicePtr);

    std::cout << "[VideoEncoder] Initializing hardware H.264 encoder (Media Foundation MFT) "
              << m_config.width << "x" << m_config.height << " @" << m_config.fps << "fps, Bitrate: "
              << m_config.bitrate << "bps..." << std::endl;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_encoder));
    if (FAILED(hr) || !m_encoder) return false;

    // Set Low-Latency Rate Control and Quality
    ICodecAPI* pCodecApi = nullptr;
    hr = m_encoder->QueryInterface(IID_PPV_ARGS(&pCodecApi));
    if (SUCCEEDED(hr)) {
        VARIANT var = {0};
        var.vt = VT_BOOL;
        var.boolVal = m_config.lowLatency ? VARIANT_TRUE : VARIANT_FALSE;
        pCodecApi->SetValue(&CODECAPI_AVLowLatencyMode, &var);

        VARIANT varBitrate = {0};
        varBitrate.vt = VT_UI4;
        varBitrate.ulVal = m_config.bitrate;
        pCodecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &varBitrate);
        pCodecApi->Release();
    }

    // Configure Output Media Type (H.264)
    hr = MFCreateMediaType(&m_outputType);
    if (FAILED(hr)) return false;
    m_outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    m_outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    m_outputType->SetUINT32(MF_MT_AVG_BITRATE, m_config.bitrate);
    MFSetAttributeSize(m_outputType, MF_MT_FRAME_SIZE, m_config.width, m_config.height);
    MFSetAttributeRatio(m_outputType, MF_MT_FRAME_RATE, m_config.fps, 1);
    MFSetAttributeRatio(m_outputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    m_outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    m_outputType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);

    hr = m_encoder->SetOutputType(0, m_outputType, 0);
    if (FAILED(hr)) return false;

    // Configure Input Media Type (NV12 / RGB32)
    hr = MFCreateMediaType(&m_inputType);
    if (FAILED(hr)) return false;
    m_inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    m_inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeSize(m_inputType, MF_MT_FRAME_SIZE, m_config.width, m_config.height);
    MFSetAttributeRatio(m_inputType, MF_MT_FRAME_RATE, m_config.fps, 1);
    MFSetAttributeRatio(m_inputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    m_inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    hr = m_encoder->SetInputType(0, m_inputType, 0);
    if (FAILED(hr)) return false;

    m_initialized = true;
    return true;
}

bool VideoEncoder::EncodeFrame(void* texturePtr, std::vector<uint8_t>& outBitstream) {
    outBitstream.clear();
    ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(texturePtr);
    if (!m_initialized || !m_device || !texture || !m_encoder) return false;

    IMFSample* pSample = nullptr;
    IMFMediaBuffer* pBuffer = nullptr;
    HRESULT hr = MFCreateSample(&pSample);
    if (FAILED(hr)) return false;

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
#else
// Cross-platform mock implementation for CI and non-Windows targets
bool VideoEncoder::Initialize(void* devicePtr, const VideoEncoderConfig& config) {
    m_config = config;
    m_initialized = true;
    std::cout << "[VideoEncoder] (Cross-Platform) Initialized video encoder simulator: "
              << m_config.width << "x" << m_config.height << " @" << m_config.fps << "fps" << std::endl;
    return true;
}

bool VideoEncoder::EncodeFrame(void* texturePtr, std::vector<uint8_t>& outBitstream) {
    if (!m_initialized) return false;
    outBitstream.clear();

    // Generate a valid mock H.264 Annex B NAL unit header (0x00, 0x00, 0x00, 0x01, 0x67 for SPS, 0x68 for PPS, 0x65 for IDR)
    const uint8_t mockH264Frame[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1f, // Mock SPS
        0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80, // Mock PPS
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00  // Mock IDR slice
    };
    outBitstream.assign(mockH264Frame, mockH264Frame + sizeof(mockH264Frame));
    return true;
}
#endif
