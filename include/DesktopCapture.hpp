#pragma once
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi1_2.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

class DesktopCapture {
public:
    DesktopCapture();
    ~DesktopCapture();

bool Initialize(void* display = nullptr);
    bool AcquireFrame();
    void ReleaseFrame();

#ifdef _WIN32
    ID3D11Texture2D* GetCurrentFrameTexture() { return m_currentFrame; }
#endif

#ifdef __linux__
    XImage* GetCurrentFrameImage() { return m_currentImage; }
#endif

private:
#ifdef _WIN32
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGIOutputDuplication* m_deskDupl;
    ID3D11Texture2D* m_currentFrame;
#endif

#ifdef __linux__
    Display* m_display;
    Window m_root;
    XImage* m_currentImage;
    int m_width;
    int m_height;
#endif
};
