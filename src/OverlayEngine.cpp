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
#endif

    m_active = true;
    return true;
}

void OverlayEngine::Render(int cursorX, int cursorY) {
    if (!m_active) return;

#ifdef _WIN32
    HWND hwnd = (HWND)m_hwnd;
    HDC hdc = GetDC(hwnd);

    // Clear previous or just draw at new position
    // For a simple GDI cursor:
    RECT rect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    // Draw a standard mouse cursor icon
    HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);
    DrawIcon(hdc, cursorX, cursorY, hCursor);

    ReleaseDC(hwnd, hdc);
#endif
}

void OverlayEngine::Shutdown() {
    if (m_active) {
        std::cout << "[Overlay] Shutting down overlay..." << std::endl;
#ifdef _WIN32
        if (m_hwnd) {
            DestroyWindow((HWND)m_hwnd);
            m_hwnd = nullptr;
        }
#endif
        m_active = false;
    }
}
