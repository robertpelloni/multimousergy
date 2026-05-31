#include "ConfigGUI.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <mutex>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

static AppSettings* s_currentSettings = nullptr;
static SyncModule* s_currentSync = nullptr;
static NetworkManager* s_currentNetwork = nullptr;
static bool s_isRunning = false;
static bool s_restartRequested = false;

#ifdef _WIN32
static HWND s_hwndMain = nullptr;
static HWND s_hwndServer = nullptr;
static HWND s_hwndIp = nullptr;
static HWND s_hwndPort = nullptr;
static HWND s_hwndBoundary = nullptr;
static HWND s_hwndPeerList = nullptr;
static HWND s_hwndLatency = nullptr;
static HWND s_hwndActiveUser = nullptr;
static HWND s_hwndCursorScale = nullptr;
static HWND s_hwndUseD3D11 = nullptr;
static HWND s_hwndNetMuxDriverType = nullptr;
static HWND s_hwndGroupId = nullptr;
static HWND s_hwndGroupName = nullptr;
static HWND s_hwndSessionName = nullptr;
static HWND s_hwndSecurityKey = nullptr;
static HWND s_hwndCursorThemePath = nullptr;
static HWND s_hwndCursorMonitor = nullptr;
static HWND s_hwndSecurityLog = nullptr;
static HWND s_hwndDiscoveryList = nullptr;

enum ControlIDs {
    ID_SAVE_BUTTON = 1,
    ID_PEER_LIST = 2,
    ID_CURSOR_SCALE = 3,
    ID_USE_D3D11 = 4,
    ID_GROUP_ID = 5,
    ID_SESSION_NAME = 6,
    ID_GROUP_NAME = 7,
    ID_SECURITY_KEY = 8,
    ID_CURSOR_THEME_PATH = 14,
    ID_CURSOR_MONITOR = 9,
    ID_DRIVER_TYPE = 10,
    ID_SECURITY_LOG = 11,
    ID_SCAN_BUTTON = 12,
    ID_DISCOVERY_LIST = 13
};

static HWND s_hwndSecurityStatus = nullptr;

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateWindow("STATIC", "Role:", WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndServer = CreateWindow("BUTTON", "Server Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 110, 10, 150, 20, hwnd, NULL, NULL, NULL);
            if (s_currentSettings->isServer) SendMessage(s_hwndServer, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindow("STATIC", "Remote IP:", WS_VISIBLE | WS_CHILD, 10, 40, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndIp = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", s_currentSettings->remoteIp.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 110, 40, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Port:", WS_VISIBLE | WS_CHILD, 10, 70, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndPort = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", std::to_string(s_currentSettings->port).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER, 110, 70, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Boundary X:", WS_VISIBLE | WS_CHILD, 10, 100, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndBoundary = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", std::to_string(s_currentSettings->inputConfig.boundaryX).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER, 110, 100, 150, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("BUTTON", "Save & Start", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 140, 120, 30, hwnd, (HMENU)ID_SAVE_BUTTON, NULL, NULL);
            CreateWindow("BUTTON", "Scan Servers", WS_VISIBLE | WS_CHILD, 140, 140, 120, 30, hwnd, (HMENU)ID_SCAN_BUTTON, NULL, NULL);

            CreateWindow("STATIC", "Discovered Servers:", WS_VISIBLE | WS_CHILD, 10, 180, 150, 20, hwnd, NULL, NULL, NULL);
            s_hwndDiscoveryList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER | LBS_NOTIFY, 10, 200, 250, 60, hwnd, (HMENU)ID_DISCOVERY_LIST, NULL, NULL);

            CreateWindow("STATIC", "Network Metrics:", WS_VISIBLE | WS_CHILD, 10, 270, 120, 20, hwnd, NULL, NULL, NULL);
            s_hwndPeerList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 10, 290, 250, 60, hwnd, (HMENU)ID_PEER_LIST, NULL, NULL);

            CreateWindow("STATIC", "Cursor Scale:", WS_VISIBLE | WS_CHILD, 10, 360, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorScale = CreateWindow("EDIT", "100", WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 110, 360, 50, 20, hwnd, (HMENU)ID_CURSOR_SCALE, NULL, NULL);

            s_hwndUseD3D11 = CreateWindow("BUTTON", "Hardware Acceleration (D3D11)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 390, 250, 20, hwnd, (HMENU)ID_USE_D3D11, NULL, NULL);
            if (s_currentSettings->useD3D11) SendMessage(s_hwndUseD3D11, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindow("STATIC", "Driver:", WS_VISIBLE | WS_CHILD, 10, 420, 60, 20, hwnd, NULL, NULL, NULL);
            s_hwndNetMuxDriverType = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 70, 420, 100, 100, hwnd, (HMENU)ID_DRIVER_TYPE, NULL, NULL);
            SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"Auto");
            SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"Interception");
            SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"ViGEmBus");
            SendMessage(s_hwndNetMuxDriverType, CB_SETCURSEL, (WPARAM)s_currentSettings->driverType, 0);

            CreateWindow("STATIC", "Group ID:", WS_VISIBLE | WS_CHILD, 180, 420, 60, 20, hwnd, NULL, NULL, NULL);
            s_hwndGroupId = CreateWindow("EDIT", std::to_string(s_currentSettings->groupId).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 245, 420, 50, 20, hwnd, (HMENU)ID_GROUP_ID, NULL, NULL);

            CreateWindow("STATIC", "Group Name:", WS_VISIBLE | WS_CHILD, 10, 450, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndGroupName = CreateWindow("EDIT", s_currentSettings->groupName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 110, 450, 150, 20, hwnd, (HMENU)ID_GROUP_NAME, NULL, NULL);

            CreateWindow("STATIC", "Session:", WS_VISIBLE | WS_CHILD, 10, 480, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSessionName = CreateWindow("EDIT", s_currentSettings->sessionName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 110, 480, 150, 20, hwnd, (HMENU)ID_SESSION_NAME, NULL, NULL);

            CreateWindow("STATIC", "Security Key:", WS_VISIBLE | WS_CHILD, 10, 510, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSecurityKey = CreateWindow("EDIT", s_currentSettings->securityKey.c_str(), WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_BORDER, 110, 510, 150, 20, hwnd, (HMENU)ID_SECURITY_KEY, NULL, NULL);

            CreateWindow("STATIC", "Cursor Theme:", WS_VISIBLE | WS_CHILD, 10, 540, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorThemePath = CreateWindow("EDIT", s_currentSettings->cursorThemePath.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 110, 540, 150, 20, hwnd, (HMENU)ID_CURSOR_THEME_PATH, NULL, NULL);

            s_hwndSecurityStatus = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD, 10, 565, 280, 20, hwnd, NULL, NULL, NULL);
            if (s_currentSettings->securityKey.empty()) SetWindowText(s_hwndSecurityStatus, "System Status: UNSECURED");
            else SetWindowText(s_hwndSecurityStatus, "System Status: ENCRYPTED (Mutual Auth)");

            CreateWindow("STATIC", "Real-Time Monitor:", WS_VISIBLE | WS_CHILD, 10, 585, 150, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorMonitor = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | WS_BORDER, 10, 605, 280, 100, hwnd, (HMENU)ID_CURSOR_MONITOR, NULL, NULL);

            CreateWindow("STATIC", "Security Log:", WS_VISIBLE | WS_CHILD, 10, 715, 150, 20, hwnd, NULL, NULL, NULL);
            s_hwndSecurityLog = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 10, 735, 280, 100, hwnd, (HMENU)ID_SECURITY_LOG, NULL, NULL);
            break;

        case WM_DRAWITEM:
            if (wParam == ID_CURSOR_MONITOR) {
                LPDRAWITEMSTRUCT pdi = (LPDRAWITEMSTRUCT)lParam;
                FillRect(pdi->hDC, &pdi->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_SCAN_BUTTON) {
                if (s_currentNetwork) {
                    SendMessage(s_hwndDiscoveryList, LB_RESETCONTENT, 0, 0);
                    ConfigGUI::LogSecurityEvent("Scanning for peers on network...");
                    // No need to block, Tick() will poll
                }
            }

            if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == ID_DISCOVERY_LIST) {
                int index = (int)SendMessage(s_hwndDiscoveryList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    char buffer[256];
                    SendMessage(s_hwndDiscoveryList, LB_GETTEXT, index, (LPARAM)buffer);
                    std::string s(buffer);
                    size_t sep = s.find(" | ");
                    if (sep != std::string::npos) {
                        std::string ip = s.substr(sep + 3);
                        SetWindowText(s_hwndIp, ip.c_str());
                        ConfigGUI::LogSecurityEvent("Auto-filled IP: " + ip);
                    }
                }
            }

            if (LOWORD(wParam) == ID_SAVE_BUTTON) {
                char buffer[256];
                try {
                    GetWindowText(s_hwndPort, buffer, 256);
                    s_currentSettings->port = std::stoi(buffer);

                    GetWindowText(s_hwndBoundary, buffer, 256);
                    s_currentSettings->inputConfig.boundaryX = std::stoi(buffer);

                    GetWindowText(s_hwndCursorScale, buffer, 256);
                    s_currentSettings->cursorScale = (float)std::stoi(buffer) / 100.0f;

                    s_currentSettings->isServer = (SendMessage(s_hwndServer, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->useD3D11 = (SendMessage(s_hwndUseD3D11, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->driverType = (NetMuxDriverType)SendMessage(s_hwndNetMuxDriverType, CB_GETCURSEL, 0, 0);

                    GetWindowText(s_hwndGroupId, buffer, 256);
                    s_currentSettings->groupId = (unsigned int)std::stoul(buffer);

                    GetWindowText(s_hwndGroupName, buffer, 256);
                    s_currentSettings->groupName = buffer;

                    GetWindowText(s_hwndSessionName, buffer, 256);
                    s_currentSettings->sessionName = buffer;

                    GetWindowText(s_hwndSecurityKey, buffer, 256);
                    s_currentSettings->securityKey = buffer;

                    GetWindowText(s_hwndIp, buffer, 256);
                    s_currentSettings->remoteIp = buffer;

                    GetWindowText(s_hwndCursorThemePath, buffer, 256);
                    s_currentSettings->cursorThemePath = buffer;

                    // Signal main thread to re-initialize
                    s_restartRequested = true;
                    PostMessage(hwnd, WM_USER + 1, 0, 0);
                } catch (...) {}
            }
            break;

        case WM_CLOSE:
            s_isRunning = false;
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

static std::mutex s_monitorMutex;

void ConfigGUI::Initialize(AppSettings& settings, SyncModule* sync, NetworkManager* network) {
    s_currentSettings = &settings;
    s_currentSync = sync;
    s_currentNetwork = network;
    s_restartRequested = false;

#ifdef _WIN32
    WNDCLASS wc = {0};
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "NetMuxIntegratedGUI";
    RegisterClass(&wc);

    s_hwndMain = CreateWindow("NetMuxIntegratedGUI", "NetMux Monitor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 320, 880, NULL, NULL, GetModuleHandle(NULL), NULL);
    s_isRunning = (s_hwndMain != NULL);
#else
    s_isRunning = true;
#endif
}

void ConfigGUI::Tick() {
#ifdef _WIN32
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) s_isRunning = false;
    }

    if (s_isRunning && s_currentNetwork) {
        DiscoveryPacket dp;
        if (s_currentNetwork->PollDiscovery(dp)) {
            char info[256];
            snprintf(info, sizeof(info), "%s | %s", dp.hostname, dp.ip);
            // Simple check to prevent duplicates
            if (SendMessage(s_hwndDiscoveryList, LB_FINDSTRINGEXACT, -1, (LPARAM)info) == LB_ERR) {
                SendMessage(s_hwndDiscoveryList, LB_ADDSTRING, 0, (LPARAM)info);
            }
        }
    }

    if (s_isRunning && s_currentSync) {
        auto peers = s_currentSync->GetAllPeers();

        // Update ListBox metrics
        SendMessage(s_hwndPeerList, LB_RESETCONTENT, 0, 0);
        for (auto const& [id, peer] : peers) {
            char info[256];
            char btnStr[16] = "---";
            if (peer.buttonState & 1) btnStr[0] = 'L';
            if (peer.buttonState & 2) btnStr[1] = 'R';
            if (peer.buttonState & 4) btnStr[2] = 'M';

            const char* authStatus = "LOCKED";
            if (id == s_currentSync->GetLocalId() || s_currentSettings->securityKey.empty()) authStatus = "OPEN";
            else if (peer.isAuthenticated) authStatus = "AUTH";

            snprintf(info, sizeof(info), "%s | RTT:%dms | E2E:%dms | Drift:%.1fpx | [%s] | %s %s",
                peer.sessionName, (int)peer.latency, (int)peer.e2eLatency, peer.drift,
                authStatus,
                btnStr, peer.isSelecting ? "[Sel]" : "");
            SendMessage(s_hwndPeerList, LB_ADDSTRING, 0, (LPARAM)info);
        }
    }
#endif
}

bool ConfigGUI::IsRunning() {
    return s_isRunning;
}

bool ConfigGUI::RestartRequested() {
    return s_restartRequested;
}

void ConfigGUI::Shutdown() {
#ifdef _WIN32
    if (s_hwndMain) DestroyWindow(s_hwndMain);
#endif
    s_isRunning = false;
}

void ConfigGUI::LogSecurityEvent(const std::string& event) {
#ifdef _WIN32
    if (s_hwndSecurityLog) {
        int count = (int)SendMessage(s_hwndSecurityLog, LB_GETCOUNT, 0, 0);
        if (count > 100) SendMessage(s_hwndSecurityLog, LB_DELETESTRING, 0, 0);

        SendMessage(s_hwndSecurityLog, LB_ADDSTRING, 0, (LPARAM)event.c_str());
        count = (int)SendMessage(s_hwndSecurityLog, LB_GETCOUNT, 0, 0);
        SendMessage(s_hwndSecurityLog, LB_SETCURSEL, count - 1, 0);
    }
#endif
    std::cout << "[Security Log] " << event << std::endl;
}

void ConfigGUI::UpdateCursorMonitor(const std::map<unsigned long long, PeerState>& peers) {
#ifdef _WIN32
    if (s_hwndCursorMonitor) {
        RECT rect;
        GetClientRect(s_hwndCursorMonitor, &rect);
        HDC hdc = GetDC(s_hwndCursorMonitor);
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        for (auto const& [id, peer] : peers) {
            int mx = (int)((peer.normalizedX * (rect.right - 10)) / 65535) + 5;
            int my = (int)((peer.normalizedY * (rect.bottom - 10)) / 65535) + 5;
            HBRUSH hBrush = CreateSolidBrush(RGB(peer.colorR, peer.colorG, peer.colorB));
            RECT cRect = { mx - 2, my - 2, mx + 2, my + 2 };
            FillRect(hdc, &cRect, hBrush);
            DeleteObject(hBrush);
        }
        ReleaseDC(s_hwndCursorMonitor, hdc);
    }
#endif
}

bool ConfigGUI::ShowDialog(AppSettings& settings, SyncModule* sync) {
    s_currentSettings = &settings;
    s_currentSync = sync;
    Initialize(settings, sync);

#ifdef _WIN32
    MSG msg;
    while (s_isRunning) {
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_USER + 1) break; // Re-init signal
        } else break;
    }
#endif
    return true;
}
