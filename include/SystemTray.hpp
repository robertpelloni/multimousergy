#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <functional>

// Tray icon context menu IDs
#define WM_TRAYICON        (WM_USER + 100)
#define ID_TRAY_SHOW_STATS  3001
#define ID_TRAY_TOGGLE_OVL  3002
#define ID_TRAY_ABOUT       3003
#define ID_TRAY_EXIT        3004

class SystemTray {
public:
    SystemTray();
    ~SystemTray();

    // Create the tray icon and hidden message window.
    // onExit is called when the user selects "Exit" from the tray menu.
    bool Initialize(HINSTANCE hInstance, const std::string& tooltip, std::function<void()> onExit);
    void Shutdown();

    // Update the hover tooltip (e.g. "MultiMousergy - 3 peers connected")
    void SetTooltip(const std::string& text);

    // Update the tray icon's info balloon
    void ShowBalloon(const std::string& title, const std::string& text, int timeoutMs = 3000);

    // Call this from the main message pump to dispatch tray messages.
    // Returns true if the message was consumed.
    bool HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // Process any pending messages for the tray's hidden window.
    void PumpMessages();

private:
    HWND            m_hwnd;
    HICON           m_hIcon;
    NOTIFYICONDATAA m_nid;
    std::function<void()> m_onExit;
    HMENU           m_hMenu;
    bool            m_initialized;

    // Hidden window that receives tray icon messages
    static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ShowContextMenu(int x, int y);
    HICON CreateMouseCursorIcon();

    // Keep a static pointer so the wndproc can reach the instance
    static SystemTray* s_instance;
};

#endif // _WIN32
