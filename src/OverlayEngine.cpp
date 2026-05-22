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
        // Render the cursor bitmap
        HDC hdcCursor = CreateCompatibleDC(hdcScreen);
        SelectObject(hdcCursor, (HBITMAP)m_hCursorBitmap);

        // Alpha blending or simple BitBlt with color keying
        // Since we are using a simple color key, we can use TransparentBlt if available,
        // or just BitBlt if we don't mind the black background (which is color-keyed anyway)
        BitBlt(hdcMem, peer.x, peer.y, m_cursorWidth, m_cursorHeight, hdcCursor, 0, 0, SRCPAINT);

        DeleteDC(hdcCursor);

        // Also draw a small colorized indicator
        HBRUSH hBrushColor = CreateSolidBrush(RGB(peer.r, peer.g, peer.b));
        RECT indicatorRect = { peer.x, peer.y, peer.x + 4, peer.y + 4 };
        FillRect(hdcMem, &indicatorRect, hBrushColor);
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
            if (m_hCursorBitmap) DeleteObject((HBITMAP)m_hCursorBitmap);
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
