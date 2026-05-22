#include "OverlayEngine.hpp"
#include <iostream>

#ifdef _WIN32
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
    m_hdcMem = nullptr;
    m_hBitmap = nullptr;
    m_hOldBitmap = nullptr;
    m_hPen = nullptr;
    m_hBrush = nullptr;
    m_screenWidth = 0;
    m_screenHeight = 0;
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

    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        "NetMuxOverlay", "NetMux Overlay",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!m_hwnd) return false;

    // Use a color key for transparency (e.g., black is transparent)
    SetLayeredWindowAttributes((HWND)m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow((HWND)m_hwnd, SW_SHOW);

    // Pre-allocate resources for flicker-free rendering
    m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
    HDC hdcScreen = GetDC(NULL);
    m_hdcMem = CreateCompatibleDC(hdcScreen);
    m_hBitmap = CreateCompatibleBitmap(hdcScreen, m_screenWidth, m_screenHeight);
    m_hOldBitmap = SelectObject((HDC)m_hdcMem, (HBITMAP)m_hBitmap);
    m_hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    m_hBrush = CreateSolidBrush(RGB(0, 0, 0));
    ReleaseDC(NULL, hdcScreen);
#endif

    m_active = true;
    return true;
}

void OverlayEngine::Render(int cursorX, int cursorY) {
    std::map<unsigned long long, RemoteCursorState> peers;
    peers[0] = { cursorX, cursorY, m_colorR, m_colorG, m_colorB };
    RenderPeers(peers);
}

void OverlayEngine::RenderPeers(const std::map<unsigned long long, RemoteCursorState>& peers) {
    if (!m_active) return;

#ifdef _WIN32
    HWND hwnd = (HWND)m_hwnd;
    HDC hdcMem = (HDC)m_hdcMem;
    HDC hdcScreen = GetDC(NULL);

    // Clear with transparent color (black + color key)
    RECT rect = { 0, 0, m_screenWidth, m_screenHeight };
    FillRect(hdcMem, &rect, (HBRUSH)m_hBrush);

    for (auto const& [id, peer] : peers) {
        // Draw a colorized crosshair or proxy for each peer
        HBRUSH hBrushColor = CreateSolidBrush(RGB(peer.r, peer.g, peer.b));
        RECT cursorRect = { peer.x - 5, peer.y - 5, peer.x + 5, peer.y + 5 };
        FillRect(hdcMem, &cursorRect, hBrushColor);

        HPEN hOldPen = (HPEN)SelectObject(hdcMem, (HPEN)m_hPen);
        MoveToEx(hdcMem, peer.x - 10, peer.y, NULL);
        LineTo(hdcMem, peer.x + 10, peer.y);
        MoveToEx(hdcMem, peer.x, peer.y - 10, NULL);
        LineTo(hdcMem, peer.x, peer.y + 10);

        SelectObject(hdcMem, hOldPen);
        DeleteObject(hBrushColor);
    }

    POINT ptSrc = { 0, 0 };
    POINT ptDest = { 0, 0 };
    SIZE size = { m_screenWidth, m_screenHeight };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hwnd, hdcScreen, &ptDest, &size, hdcMem, &ptSrc, RGB(0,0,0), &blend, ULW_COLORKEY);

    ReleaseDC(NULL, hdcScreen);
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
            DeleteDC((HDC)m_hdcMem);
        }
        if (m_hwnd) {
            DestroyWindow((HWND)m_hwnd);
            m_hwnd = nullptr;
        }
#endif
        m_active = false;
    }
}
