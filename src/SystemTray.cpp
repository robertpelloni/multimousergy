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
    // Create a 16x16 mouse cursor icon programmatically
    // We draw an arrow cursor with a hand-like appearance
    const int w = 16, h = 16;

    // Create a monochrome bitmap for the icon (AND mask)
    HBITMAP hAndMask = CreateBitmap(w, h, 1, 1, nullptr);
    // Create an RGBA bitmap for the icon (XOR mask / color)
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcColor = CreateCompatibleDC(hdcScreen);
    HDC hdcMask  = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h; // top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* pBits = nullptr;
    HBITMAP hColor = CreateDIBSection(hdcColor, &bi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    HBITMAP hOldColor = (HBITMAP)SelectObject(hdcColor, hColor);
    HBITMAP hOldMask  = (HBITMAP)SelectObject(hdcMask, hAndMask);

    // Draw the AND mask (white = visible, black = transparent)
    RECT rc = { 0, 0, w, h };
    FillRect(hdcMask, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

    // Fill XOR layer with cyan (the icon color)
    HBRUSH hCyanBrush = CreateSolidBrush(RGB(0, 200, 255));
    FillRect(hdcColor, &rc, hCyanBrush);
    DeleteObject(hCyanBrush);

    // Draw a dark arrow on top
    HPEN hDarkPen = CreatePen(PS_SOLID, 1, RGB(20, 40, 80));
    HPEN hOldPen = (HPEN)SelectObject(hdcColor, hDarkPen);

    // Arrow body
    POINT arrow[] = {
        { 3,  1 },   // tip
        { 3,  12 },  // down
        { 6,  9 },   // right inner
        { 9,  13 },  // right tail
        { 11, 12 },  // right tail end
        { 8,  8 },   // right outer
        { 12, 8 },   // right horizontal
    };
    Polygon(hdcColor, arrow, 7);

    // Arrow outline (dark border for clarity)
    HPEN hOutlinePen = CreatePen(PS_SOLID, 1, RGB(10, 20, 40));
    SelectObject(hdcColor, hOutlinePen);
    POINT outline[] = {
        { 3,  1 },
        { 3,  12 },
        { 6,  9 },
        { 9,  13 },
        { 11, 12 },
        { 8,  8 },
        { 12, 8 },
    };
    Polyline(hdcColor, outline, 7);

    SelectObject(hdcColor, hOldPen);
    SelectObject(hdcColor, hOldColor);
    SelectObject(hdcMask, hOldMask);
    DeleteObject(hDarkPen);
    DeleteObject(hOutlinePen);
    DeleteDC(hdcColor);
    DeleteDC(hdcMask);
    ReleaseDC(nullptr, hdcScreen);

    ICONINFO ii = {};
    ii.fIcon    = TRUE;
    ii.hbmMask  = hAndMask;
    ii.hbmColor = hColor;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hAndMask);
    DeleteObject(hColor);

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

    m_hwnd = CreateWindowEx(
        0, "NetMuxTrayWnd", "NetMux Tray",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, hInstance, nullptr
    );
    if (!m_hwnd) {
        std::cerr << "[Tray] Failed to create message window." << std::endl;
        return false;
    }

    // Store a pointer to this instance in the window's user data
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Create the mouse cursor icon
    m_hIcon = CreateMouseCursorIcon();
    if (!m_hIcon) {
        // Fallback: use the system arrow cursor
        m_hIcon = (HICON)LoadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 16, 16, LR_DEFAULTCOLOR);
        std::cerr << "[Tray] Custom icon failed, using system arrow." << std::endl;
    }

    // Set up the NOTIFYICONDATA structure
    m_nid.cbSize           = sizeof(NOTIFYICONDATAA);
    m_nid.hWnd             = m_hwnd;
    m_nid.uID              = 1;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon            = m_hIcon;
    strncpy_s(m_nid.szTip, sizeof(m_nid.szTip), tooltip.c_str(), _TRUNCATE);

    if (!Shell_NotifyIconA(NIM_ADD, &m_nid)) {
        std::cerr << "[Tray] Shell_NotifyIcon(NIM_ADD) failed." << std::endl;
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
                    "Cross-Network Multi-Cursor System\nv0.1.89-alpha",
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
                    "System is running.\nVersion: v0.1.89-alpha",
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
