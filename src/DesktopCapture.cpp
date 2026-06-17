#include "DesktopCapture.hpp"
#include <iostream>

DesktopCapture::DesktopCapture() {
#ifdef _WIN32
    m_device = nullptr;
    m_context = nullptr;
    m_deskDupl = nullptr;
    m_currentFrame = nullptr;
#endif
#ifdef __linux__
    m_display = nullptr;
    m_currentImage = nullptr;
#endif
}

DesktopCapture::~DesktopCapture() {
#ifdef _WIN32
    if (m_deskDupl) m_deskDupl->Release();
#endif
#ifdef __linux__
    if (m_currentImage) XDestroyImage(m_currentImage);
#endif
}

#ifdef __linux__
bool DesktopCapture::Initialize(void* display) {
    m_display = (Display*)display;
    if (!m_display) return false;
    m_root = DefaultRootWindow(m_display);
    XWindowAttributes gwa;
    XGetWindowAttributes(m_display, m_root, &gwa);
    m_width = gwa.width;
    m_height = gwa.height;
    std::cout << "[DesktopCapture] Initialized Linux X11 capture: " << m_width << "x" << m_height << std::endl;
    return true;
}
#else
bool DesktopCapture::Initialize() {
#ifdef _WIN32
    std::cout << "[DesktopCapture] Initializing DXGI Desktop Duplication..." << std::endl;

    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_device, NULL, &m_context);
    if (FAILED(hr)) return false;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
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
    if (FAILED(hr)) return false;

    return true;
#else
    return false;
#endif
}
#endif

bool DesktopCapture::AcquireFrame() {
#ifdef __linux__
    if (!m_display) return false;
    if (m_currentImage) XDestroyImage(m_currentImage);
    m_currentImage = XGetImage(m_display, m_root, 0, 0, m_width, m_height, AllPlanes, ZPixmap);
    return m_currentImage != nullptr;
#endif
#ifdef _WIN32
    if (!m_deskDupl) return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = nullptr;
    HRESULT hr = m_deskDupl->AcquireNextFrame(100, &frameInfo, &desktopResource);
    if (FAILED(hr)) return false;

    if (m_currentFrame) m_currentFrame->Release();
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&m_currentFrame);
    desktopResource->Release();

    return SUCCEEDED(hr);
#else
    return false;
#endif
}

void DesktopCapture::ReleaseFrame() {
#ifdef _WIN32
    if (m_deskDupl) {
        m_deskDupl->ReleaseFrame();
    }
#endif
}
