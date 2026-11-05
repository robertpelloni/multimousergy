#include "SystemTray.hpp"

#ifdef _WIN32

#include <shellapi.h>
#include <iostream>

// Static instance pointer for the window procedure
SystemTray* SystemTray::s_instance = nullptr;

SystemTray::SystemTray()
    : m_hwnd(nullptr)
    , m_hIcon(nullptr)
    , m_hMenu(nullptr)
    , m_initialized(false)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
}

SystemTray::~SystemTray() {
    Shutdown();
}

HICON SystemTray::CreateMouseCursorIcon() {
    // Draw a 16x16 mouse cursor icon using GDI + CreateIconIndirect
    const int w = 16, h = 16;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcColor = CreateCompatibleDC(hdcScreen);
    HDC hdcMask = CreateCompatibleDC(hdcScreen);

    // Color bitmap (32-bit BGRA)
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* pColorBits = NULL;
    HBITMAP hColor = CreateDIBSection(hdcColor, &bmi, DIB_RGB_COLORS, &pColorBits, NULL, 0);

    // AND mask (1bpp monochrome)
    HBITMAP hMask = CreateBitmap(w, h, 1, 1, NULL);

    HBITMAP hOldC = (HBITMAP)SelectObject(hdcColor, hColor);
    HBITMAP hOldM = (HBITMAP)SelectObject(hdcMask, hMask);

    // AND mask: all zeros = all pixels come from XOR mask
    RECT rc = {0, 0, w, h};
    FillRect(hdcMask, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Fill with cyan background
    HBRUSH hCyan = CreateSolidBrush(RGB(0, 180, 230));
    FillRect(hdcColor, &rc, hCyan);
    DeleteObject(hCyan);

    // Draw mouse cursor shape (dark blue outline, white fill)
    // Body: rounded rectangle
    HBRUSH hBody = CreateSolidBrush(RGB(40, 60, 120));
    HPEN hOutline = CreatePen(PS_SOLID, 1, RGB(20, 30, 60));
    HPEN hOldP = (HPEN)SelectObject(hdcColor, hOutline);
    HBRUSH hOldB = (HBRUSH)SelectObject(hdcColor, hBody);
    RoundRect(hdcColor, 4, 2, 12, 14, 4, 4);

    // Divider line (separate left/right buttons)
    HPEN hDiv = CreatePen(PS_SOLID, 1, RGB(20, 30, 60));
    SelectObject(hdcColor, hDiv);
    MoveToEx(hdcColor, 8, 2, NULL);
    LineTo(hdcColor, 8, 8);
    DeleteObject(hDiv);

    // Scroll wheel (small rectangle)
    HBRUSH hWheel = CreateSolidBrush(RGB(100, 140, 200));
    SelectObject(hdcColor, hWheel);
    Rectangle(hdcColor, 7, 3, 9, 7);
    DeleteObject(hWheel);

    // Restore and clean up GDI objects
    SelectObject(hdcColor, hOldP);
    SelectObject(hdcColor, hOldB);
    DeleteObject(hBody);
    DeleteObject(hOutline);
    SelectObject(hdcColor, hOldC);
    SelectObject(hdcMask, hOldM);
    DeleteDC(hdcColor);
    DeleteDC(hdcMask);
    ReleaseDC(NULL, hdcScreen);

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hMask;
    ii.hbmColor = hColor;
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(hMask);
    DeleteObject(hColor);

    if (!hIcon) {
        std::cerr << "[Tray] CreateIconIndirect failed: " << GetLastError() << std::endl;
    }
    return hIcon;
}

bool SystemTray::Initialize(HINSTANCE hInstance, const std::string& tooltip, std::function<void()> onExit) {
    s_instance = this;
    m_onExit = onExit;

    // Register a hidden window class for tray messages
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = TrayWndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = "NetMuxTrayWnd";
    RegisterClassEx(&wc);

    // Create a message-only window for tray messages.
    m_hwnd = CreateWindowEx(
        0,
        "NetMuxTrayWnd", "NetMux Tray",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, nullptr, hInstance, nullptr
    );
    if (!m_hwnd) {
        std::cerr << "[Tray] Failed to create message window." << std::endl;
        return false;
    }

    // Store a pointer to this instance in the window's user data
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    m_hIcon = CreateMouseCursorIcon();
    if (!m_hIcon) {
        std::cerr << "[Tray] Icon creation failed." << std::endl;
        return false;
    }

    m_nid.cbSize           = sizeof(NOTIFYICONDATAA);
    m_nid.hWnd             = m_hwnd;
    m_nid.uID              = 1;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon            = m_hIcon;
    strncpy_s(m_nid.szTip, sizeof(m_nid.szTip), tooltip.c_str(), _TRUNCATE);

    Shell_NotifyIconA(NIM_DELETE, &m_nid);
    if (!Shell_NotifyIconA(NIM_ADD, &m_nid)) {
        std::cerr << "[Tray] Shell_NotifyIcon(NIM_ADD) failed: " << GetLastError() << std::endl;
        return false;
    }

    // Build the context menu
    m_hMenu = CreatePopupMenu();
    AppendMenuA(m_hMenu, MF_STRING, ID_TRAY_SHOW_STATS, "Show &Stats");
    AppendMenuA(m_hMenu, MF_STRING, ID_TRAY_TOGGLE_OVL, "Toggle &Overlay");
    AppendMenuA(m_hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(m_hMenu, MF_STRING, ID_TRAY_ABOUT, "&About MultiMousergy");
    AppendMenuA(m_hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(m_hMenu, MF_STRING, ID_TRAY_EXIT, "E&xit");

    m_initialized = true;
    std::cout << "[Tray] System tray icon initialized." << std::endl;
    return true;
}

void SystemTray::Shutdown() {
    if (!m_initialized) return;

    Shell_NotifyIconA(NIM_DELETE, &m_nid);

    if (m_hMenu) {
        DestroyMenu(m_hMenu);
        m_hMenu = nullptr;
    }
    if (m_hIcon) {
        DestroyIcon(m_hIcon);
        m_hIcon = nullptr;
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_initialized = false;
    std::cout << "[Tray] System tray icon removed." << std::endl;
}

void SystemTray::SetTooltip(const std::string& text) {
    if (!m_initialized) return;
    strncpy_s(m_nid.szTip, sizeof(m_nid.szTip), text.c_str(), _TRUNCATE);
    m_nid.uFlags = NIF_TIP;
    Shell_NotifyIconA(NIM_MODIFY, &m_nid);
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; // restore full flags
}

void SystemTray::ShowBalloon(const std::string& title, const std::string& text, int timeoutMs) {
    if (!m_initialized) return;
    m_nid.uFlags |= NIF_INFO;
    m_nid.dwInfoFlags = NIIF_INFO;
    strncpy_s(m_nid.szInfoTitle, sizeof(m_nid.szInfoTitle), title.c_str(), _TRUNCATE);
    strncpy_s(m_nid.szInfo, sizeof(m_nid.szInfo), text.c_str(), _TRUNCATE);
    m_nid.uTimeout = timeoutMs;
    Shell_NotifyIconA(NIM_MODIFY, &m_nid);
    m_nid.uFlags &= ~NIF_INFO; // restore flags
}

bool SystemTray::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        switch (LOWORD(lParam)) {
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU: {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(m_hwnd);
                ShowContextMenu(pt.x, pt.y);
                PostMessage(m_hwnd, WM_NULL, 0, 0);
                return true;
            }
            case WM_LBUTTONDBLCLK:
                // Double-click: show about
                ShowBalloon("MultiMousergy",
                    "Cross-Network Multi-Cursor System\nv0.1.90-alpha",
                    3000);
                return true;
        }
    }
    return false;
}

void SystemTray::PumpMessages() {
    MSG msg;
    while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void SystemTray::ShowContextMenu(int x, int y) {
    if (!m_hMenu) return;
    // Required: set foreground window so the menu dismisses properly
    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(m_hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
                   x, y, 0, m_hwnd, nullptr);
    PostMessage(m_hwnd, WM_NULL, 0, 0);
}

LRESULT CALLBACK SystemTray::TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SystemTray* self = reinterpret_cast<SystemTray*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (msg == WM_COMMAND && self) {
        switch (LOWORD(wParam)) {
            case ID_TRAY_SHOW_STATS:
                self->ShowBalloon("MultiMousergy Stats",
                    "System is running.\nVersion: v0.1.90-alpha",
                    3000);
                return 0;
            case ID_TRAY_TOGGLE_OVL:
                // Could toggle overlay visibility — for now just notify
                self->ShowBalloon("Overlay", "Toggle overlay (not yet wired)", 2000);
                return 0;
            case ID_TRAY_ABOUT:
                self->ShowBalloon("About MultiMousergy",
                    "Cross-Network Multi-Cursor System\n"
                    "github.com/robertpelloni/multimousergy",
                    4000);
                return 0;
            case ID_TRAY_EXIT:
                if (self->m_onExit) {
                    self->m_onExit();
                }
                return 0;
        }
    }

    if (msg == WM_TRAYICON && self) {
        self->HandleMessage(msg, wParam, lParam);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

#endif // _WIN32
