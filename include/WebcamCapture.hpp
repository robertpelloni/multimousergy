#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#endif

class WebcamCapture {
public:
    WebcamCapture();
    ~WebcamCapture();

#ifdef _WIN32
    bool Initialize(ID3D11Device* device);
    bool AcquireFrame();
    void ReleaseFrame();

    ID3D11Texture2D* GetCurrentFrameTexture() { return m_currentFrame; }
#else
    bool Initialize(void* device) { return false; }
    bool AcquireFrame() { return false; }
    void ReleaseFrame() {}
#endif

private:
#ifdef _WIN32
    ID3D11Device* m_device;
    IMFSourceReader* m_reader;
    ID3D11Texture2D* m_currentFrame;
#endif
};
