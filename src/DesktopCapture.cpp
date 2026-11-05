#include "DesktopCapture.hpp"
#include <iostream>

DesktopCapture::DesktopCapture() {
#ifdef _WIN32
    m_device = nullptr;
    m_context = nullptr;
    m_deskDupl = nullptr;
    m_currentFrame = nullptr;
    m_lastFrameTime = 0;
#else
    m_display = nullptr;
    m_rootWindow = 0;
    m_currentImage = nullptr;
    m_screenWidth = 0;
    m_screenHeight = 0;
#endif
}

DesktopCapture::~DesktopCapture() {
#ifdef _WIN32
    if (m_deskDupl) m_deskDupl->Release();
    if (m_currentFrame) m_currentFrame->Release();
    if (m_context) m_context->Release();
    if (m_device) m_device->Release();
#else
    if (m_currentImage) {
        XDestroyImage(m_currentImage);
        m_currentImage = nullptr;
    }
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
#endif
}

#ifdef _WIN32
bool DesktopCapture::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    std::cout << "[DesktopCapture] Initializing DXGI Desktop Duplication..." << std::endl;

    if (device && context) {
        m_device = device;
        m_context = context;
        m_device->AddRef();
        m_context->AddRef();
    } else {
        HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_device, NULL, &m_context);
        if (FAILED(hr)) return false;
    }

    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return false;

    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) return false;

    IDXGIOutput* dxgiOutput = nullptr;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    dxgiAdapter->Release();
    if (FAILED(hr)) return false;

    IDXGIOutput1* dxgiOutput1 = nullptr;
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);
    dxgiOutput->Release();
    if (FAILED(hr)) return false;

    hr = dxgiOutput1->DuplicateOutput(m_device, &m_deskDupl);
    dxgiOutput1->Release();
    if (FAILED(hr)) {
        std::cerr << "[DesktopCapture] DuplicateOutput failed: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }

    std::cout << "[DesktopCapture] DXGI Desktop Duplication ready." << std::endl;
    return true;
#elif defined(__linux__)
bool DesktopCapture::Initialize(Display* display) {
    std::cout << "[DesktopCapture] Initializing X11 Frame Capture..." << std::endl;
    m_display = XOpenDisplay(NULL);
    if (!m_display) {
        std::cerr << "[DesktopCapture] Failed to open X display." << std::endl;
        return false;
    }

    int screen = DefaultScreen(m_display);
    m_rootWindow = RootWindow(m_display, screen);
    m_screenWidth = DisplayWidth(m_display, screen);
    m_screenHeight = DisplayHeight(m_display, screen);

    return true;
#else
bool DesktopCapture::Initialize() {
    // Stub for non-Windows, non-Linux platforms
    return false;
}
#endif
}

bool DesktopCapture::AcquireFrame() {
#ifdef _WIN32
    if (!m_deskDupl) return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = nullptr;
    // Non-blocking: timeout 0 to avoid stalling the main loop
    HRESULT hr = m_deskDupl->AcquireNextFrame(0, &frameInfo, &desktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // No new frame available — reuse previous frame
        return (m_currentFrame != nullptr);
    }
    if (FAILED(hr)) return false;

    // Skip duplicate frames (no change since last AcquireNextFrame)
    if (frameInfo.LastPresentTime.QuadPart == m_lastFrameTime) {
        m_deskDupl->ReleaseFrame();
        desktopResource->Release();
        return (m_currentFrame != nullptr);
    }
    m_lastFrameTime = frameInfo.LastPresentTime.QuadPart;

    if (m_currentFrame) m_currentFrame->Release();
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&m_currentFrame);
    desktopResource->Release();

    return SUCCEEDED(hr);
#else
    if (!m_display) return false;

    if (m_currentImage) {
        XDestroyImage(m_currentImage);
        m_currentImage = nullptr;
    }

    m_currentImage = XGetImage(m_display, m_rootWindow, 0, 0, m_screenWidth, m_screenHeight, AllPlanes, ZPixmap);
    return m_currentImage != nullptr;
#endif
}

void DesktopCapture::ReleaseFrame() {
#ifdef _WIN32
    if (m_deskDupl) {
        m_deskDupl->ReleaseFrame();
    }
#else
    // For X11, we manage the image cleanup in ~DesktopCapture or next AcquireFrame.
    // XDestroyImage is handled during AcquireFrame to reuse/replace.
#endif
}
