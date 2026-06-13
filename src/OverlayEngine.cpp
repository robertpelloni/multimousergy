#include "OverlayEngine.hpp"
#include "D3D11Overlay.hpp"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // The window is transparent, we draw the cursor here
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

OverlayEngine::OverlayEngine() : m_active(false) {
#ifdef _WIN32
    m_hwnd = nullptr;
    m_d3dOverlay = nullptr;
    m_hdcMem = nullptr;
    m_hBitmap = nullptr;
    m_hOldBitmap = nullptr;
    m_hPen = nullptr;
    m_hBrush = nullptr;
    m_screenX = 0;
    m_screenY = 0;
    m_screenWidth = 0;
    m_screenHeight = 0;
    m_hCursorBitmap = nullptr;
    m_cursorWidth = 0;
    m_cursorHeight = 0;
#endif
}

OverlayEngine::~OverlayEngine() {
    Shutdown();
}

bool OverlayEngine::Initialize() {
    std::cout << "[Overlay] Initializing Layered Window..." << std::endl;

#ifdef _WIN32
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "NetMuxOverlay";
    RegisterClassEx(&wc);

    m_screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    m_screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    m_screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        "NetMuxOverlay", "NetMux Overlay",
        WS_POPUP,
        m_screenX, m_screenY, m_screenWidth, m_screenHeight,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!m_hwnd) return false;

    if (m_backend == OverlayBackend::D3D11) {
        m_d3dOverlay = new D3D11Overlay();
        if (!static_cast<D3D11Overlay*>(m_d3dOverlay)->Initialize((HWND)m_hwnd)) {
            std::cout << "[Overlay] D3D11 Init failed, falling back to GDI." << std::endl;
            m_backend = OverlayBackend::GDI;
        }
    }

    // Use a color key for transparency (e.g., black is transparent)
    // Note: SyncModule ensures no peer cursor uses pure black (RGB 0,0,0)
    // to prevent them from becoming transparent.
    SetLayeredWindowAttributes((HWND)m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow((HWND)m_hwnd, SW_SHOW);

    // Pre-allocate resources for flicker-free rendering
    HDC hdcScreen = GetDC(NULL);
    m_hdcMem = CreateCompatibleDC(hdcScreen);
    m_hBitmap = CreateCompatibleBitmap(hdcScreen, m_screenWidth, m_screenHeight);
    m_hOldBitmap = SelectObject((HDC)m_hdcMem, (HBITMAP)m_hBitmap);
    m_hPen = nullptr; // Reserved for future crosshair mode
    m_hBrush = CreateSolidBrush(RGB(0, 0, 0));

    // Create a default cursor bitmap (simple arrow shape)
    m_cursorWidth = 32;
    m_cursorHeight = 32;
    m_hCursorBitmap = CreateCompatibleBitmap(hdcScreen, m_cursorWidth, m_cursorHeight);
    HDC hdcCursor = CreateCompatibleDC(hdcScreen);
    HBITMAP hOldCursorBmp = (HBITMAP)SelectObject(hdcCursor, (HBITMAP)m_hCursorBitmap);

    // Fill with transparent color (black)
    RECT cRect = { 0, 0, m_cursorWidth, m_cursorHeight };
    FillRect(hdcCursor, &cRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Draw a simple white arrow
    HPEN hWhitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HPEN hOldP = (HPEN)SelectObject(hdcCursor, hWhitePen);
    MoveToEx(hdcCursor, 0, 0, NULL);
    LineTo(hdcCursor, 15, 10);
    LineTo(hdcCursor, 10, 15);
    LineTo(hdcCursor, 0, 0);

    SelectObject(hdcCursor, hOldP);
    DeleteObject(hWhitePen);
    SelectObject(hdcCursor, hOldCursorBmp);
    DeleteDC(hdcCursor);

    ReleaseDC(NULL, hdcScreen);
#endif

    m_active = true;
    return true;
}

void OverlayEngine::Render(int cursorX, int cursorY) {
    std::map<unsigned long long, RemoteCursorState> peers;
    peers[0] = { cursorX, cursorY, m_colorR, m_colorG, m_colorB, 0 };
    RenderPeers(peers);
}

void OverlayEngine::RenderPeers(const std::map<unsigned long long, RemoteCursorState>& peers) {
    if (!m_active) return;

#ifdef _WIN32
    HWND hwnd = (HWND)m_hwnd;
    // Keep overlay on top for all backends
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    if (m_backend == OverlayBackend::D3D11 && m_d3dOverlay) {
        static_cast<D3D11Overlay*>(m_d3dOverlay)->Render(peers);
        return;
    }

    HDC hdcMem = (HDC)m_hdcMem;
    HDC hdcScreen = GetDC(NULL);

    // Clear with transparent color (black + color key)
    RECT rect = { 0, 0, m_screenWidth, m_screenHeight };
    FillRect(hdcMem, &rect, (HBRUSH)m_hBrush);

    // Filter peers to only those in our group if we are a client
    // (NetMuxFramework already does this for us before calling RenderPeers)

    for (auto const& [id, peer] : peers) {
        int rx = peer.x - m_screenX;
        int ry = peer.y - m_screenY;

        HDC hdcCursor = CreateCompatibleDC(hdcScreen);
        SelectObject(hdcCursor, (HBITMAP)m_hCursorBitmap);

        if (m_scale == 1.0f) {
            BitBlt(hdcMem, rx, ry, m_cursorWidth, m_cursorHeight, hdcCursor, 0, 0, SRCPAINT);
        } else {
            StretchBlt(hdcMem, rx, ry, (int)(m_cursorWidth * m_scale), (int)(m_cursorHeight * m_scale), hdcCursor, 0, 0, m_cursorWidth, m_cursorHeight, SRCPAINT);
        }
        DeleteDC(hdcCursor);

        HBRUSH hBrushColor = CreateSolidBrush(RGB(peer.r, peer.g, peer.b));
        RECT indicatorRect = { rx, ry, rx + 6, ry + 6 };
        FillRect(hdcMem, &indicatorRect, hBrushColor);
        DeleteObject(hBrushColor);

        if (id == m_activePeerId) {
            HPEN hHaloPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            HPEN hOldP = (HPEN)SelectObject(hdcMem, hHaloPen);
            Ellipse(hdcMem, rx - 10, ry - 10, rx + 10, ry + 10);
            SelectObject(hdcMem, hOldP);
            DeleteObject(hHaloPen);
        }
    }

    POINT ptSrc = { 0, 0 };
    POINT ptDest = { m_screenX, m_screenY };
    SIZE size = { m_screenWidth, m_screenHeight };
    UpdateLayeredWindow(hwnd, hdcScreen, &ptDest, &size, hdcMem, &ptSrc, RGB(0,0,0), NULL, ULW_COLORKEY);

    ReleaseDC(NULL, hdcScreen);
#endif
}

void OverlayEngine::SetActivePeer(unsigned long long id) {
    m_activePeerId = id;
#ifdef _WIN32
    if (m_backend == OverlayBackend::D3D11 && m_d3dOverlay) {
        static_cast<D3D11Overlay*>(m_d3dOverlay)->SetActivePeer(id);
    }
#endif
}

void OverlayEngine::SetSelectionColor(unsigned char r, unsigned char g, unsigned char b) {
    m_selR = r; m_selG = g; m_selB = b;
#ifdef _WIN32
    if (m_backend == OverlayBackend::D3D11 && m_d3dOverlay) {
        static_cast<D3D11Overlay*>(m_d3dOverlay)->SetSelectionColor(r, g, b);
    }
#endif
}

void OverlayEngine::LoadCursorTheme(const std::string& path) {
#ifdef _WIN32
    if (path.empty()) return;
    HBITMAP hNew = (HBITMAP)LoadImage(NULL, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hNew) {
        if (m_hCursorBitmap) DeleteObject((HBITMAP)m_hCursorBitmap);
        m_hCursorBitmap = hNew;
        BITMAP bm;
        GetObject(hNew, sizeof(bm), &bm);
        m_cursorWidth = bm.bmWidth;
        m_cursorHeight = bm.bmHeight;
        std::cout << "[Overlay] Loaded custom cursor theme: " << path << " (" << m_cursorWidth << "x" << m_cursorHeight << ")" << std::endl;

        if (m_backend == OverlayBackend::D3D11 && m_d3dOverlay) {
            static_cast<D3D11Overlay*>(m_d3dOverlay)->UpdateCursorTexture(m_hCursorBitmap);
        }
    } else {
        std::cerr << "[Overlay] Failed to load cursor theme: " << path << std::endl;
    }
#endif
}

void OverlayEngine::Shutdown() {
    if (m_active) {
        std::cout << "[Overlay] Shutting down overlay..." << std::endl;
#ifdef _WIN32
        if (m_hdcMem) {
            SelectObject((HDC)m_hdcMem, (HBITMAP)m_hOldBitmap);
            DeleteObject((HBITMAP)m_hBitmap);
            if (m_hPen) DeleteObject((HPEN)m_hPen);
            if (m_hBrush) DeleteObject((HBRUSH)m_hBrush);
            if (m_hCursorBitmap) DeleteObject((HBITMAP)m_hCursorBitmap);
            DeleteDC((HDC)m_hdcMem);
        }
        if (m_d3dOverlay) {
            static_cast<D3D11Overlay*>(m_d3dOverlay)->Shutdown();
            delete static_cast<D3D11Overlay*>(m_d3dOverlay);
            m_d3dOverlay = nullptr;
        }
        if (m_hwnd) {
            DestroyWindow((HWND)m_hwnd);
            m_hwnd = nullptr;
        }
#endif
        m_active = false;
    }
}
