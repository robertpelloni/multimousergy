#pragma once
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi1_2.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

class DesktopCapture {
public:
    DesktopCapture();
    ~DesktopCapture();

#ifdef _WIN32
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
#elif defined(__linux__)
    bool Initialize(Display* display);
#else
    bool Initialize();
#endif
    bool AcquireFrame();
    void ReleaseFrame();

#ifdef _WIN32
    ID3D11Texture2D* GetCurrentFrameTexture() { return m_currentFrame; }
#elif defined(__linux__)
    XImage* GetCurrentFrameImage() { return m_currentImage; }
#endif

private:
#ifdef _WIN32
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGIOutputDuplication* m_deskDupl;
    ID3D11Texture2D* m_currentFrame;
#elif defined(__linux__)
    Display* m_display;
    Window m_rootWindow;
    XImage* m_currentImage;
    int m_screenWidth;
    int m_screenHeight;
#endif
};
