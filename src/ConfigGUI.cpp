#include "ConfigGUI.hpp"
#include <iostream>
#include <thread>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static AppSettings* s_currentSettings = nullptr;
static HWND s_hwndServer = nullptr;
static HWND s_hwndIp = nullptr;
static HWND s_hwndPort = nullptr;
static HWND s_hwndBoundary = nullptr;
static HWND s_hwndPeerList = nullptr;
static HWND s_hwndLatency = nullptr;
static HWND s_hwndActiveUser = nullptr;
static HWND s_hwndCursorScale = nullptr;
static HWND s_hwndUseD3D11 = nullptr;
static HWND s_hwndGroupId = nullptr;
static HWND s_hwndGroupName = nullptr;
static HWND s_hwndSessionName = nullptr;
static HWND s_hwndSecurityKey = nullptr;
static HWND s_hwndCursorMonitor = nullptr;

enum ControlIDs {
    ID_SAVE_BUTTON = 1,
    ID_PEER_LIST = 2,
    ID_CURSOR_SCALE = 3,
    ID_USE_D3D11 = 4,
    ID_GROUP_ID = 5,
    ID_SESSION_NAME = 6,
    ID_GROUP_NAME = 7,
    ID_SECURITY_KEY = 8,
    ID_CURSOR_MONITOR = 9
};

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

            CreateWindow("BUTTON", "Save & Start", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 80, 140, 120, 30, hwnd, (HMENU)ID_SAVE_BUTTON, NULL, NULL);

            CreateWindow("STATIC", "Discovered Peers:", WS_VISIBLE | WS_CHILD, 10, 180, 120, 20, hwnd, NULL, NULL, NULL);
            s_hwndPeerList = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 110, 180, 150, 100, hwnd, (HMENU)ID_PEER_LIST, NULL, NULL);

            s_hwndLatency = CreateWindow("STATIC", "Latency: --- ms", WS_VISIBLE | WS_CHILD, 10, 210, 200, 20, hwnd, NULL, NULL, NULL);
            s_hwndActiveUser = CreateWindow("STATIC", "Active User: 0", WS_VISIBLE | WS_CHILD, 10, 240, 200, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Cursor Scale:", WS_VISIBLE | WS_CHILD, 10, 270, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorScale = CreateWindow("EDIT", "100", WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 110, 270, 50, 20, hwnd, (HMENU)ID_CURSOR_SCALE, NULL, NULL);

            s_hwndUseD3D11 = CreateWindow("BUTTON", "Hardware Acceleration (D3D11)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 300, 250, 20, hwnd, (HMENU)ID_USE_D3D11, NULL, NULL);
            if (s_currentSettings->useD3D11) SendMessage(s_hwndUseD3D11, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindow("STATIC", "Group ID:", WS_VISIBLE | WS_CHILD, 10, 330, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndGroupId = CreateWindow("EDIT", std::to_string(s_currentSettings->groupId).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 110, 330, 50, 20, hwnd, (HMENU)ID_GROUP_ID, NULL, NULL);

            CreateWindow("STATIC", "Group Name:", WS_VISIBLE | WS_CHILD, 10, 360, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndGroupName = CreateWindow("EDIT", s_currentSettings->groupName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 110, 360, 150, 20, hwnd, (HMENU)ID_GROUP_NAME, NULL, NULL);

            CreateWindow("STATIC", "Session:", WS_VISIBLE | WS_CHILD, 10, 390, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSessionName = CreateWindow("EDIT", s_currentSettings->sessionName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 110, 390, 150, 20, hwnd, (HMENU)ID_SESSION_NAME, NULL, NULL);

            CreateWindow("STATIC", "Security Key:", WS_VISIBLE | WS_CHILD, 10, 420, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSecurityKey = CreateWindow("EDIT", s_currentSettings->securityKey.c_str(), WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_BORDER, 110, 420, 150, 20, hwnd, (HMENU)ID_SECURITY_KEY, NULL, NULL);

            CreateWindow("STATIC", "Real-Time Monitor:", WS_VISIBLE | WS_CHILD, 10, 450, 150, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorMonitor = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | WS_BORDER, 10, 470, 280, 100, hwnd, (HMENU)ID_CURSOR_MONITOR, NULL, NULL);
            break;

        case WM_DRAWITEM:
            if (wParam == ID_CURSOR_MONITOR) {
                LPDRAWITEMSTRUCT pdi = (LPDRAWITEMSTRUCT)lParam;
                FillRect(pdi->hDC, &pdi->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
                // Rendering is driven by UpdateCursorMonitor
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_PEER_LIST && HIWORD(wParam) == CBN_SELCHANGE) {
                char buffer[256];
                int index = (int)SendMessage(s_hwndPeerList, CB_GETCURSEL, 0, 0);
                if (index != CB_ERR) {
                    SendMessage(s_hwndPeerList, CB_GETLBTEXT, index, (LPARAM)buffer);
                    SetWindowText(s_hwndIp, buffer);
                }
            }

            if (LOWORD(wParam) == ID_SAVE_BUTTON) {
                char buffer[256];
                try {
                    GetWindowText(s_hwndPort, buffer, 256);
                    int port = std::stoi(buffer);

                    GetWindowText(s_hwndBoundary, buffer, 256);
                    int boundaryX = std::stoi(buffer);

                    GetWindowText(s_hwndCursorScale, buffer, 256);
                    s_currentSettings->cursorScale = (float)std::stoi(buffer) / 100.0f;

                    s_currentSettings->isServer = (SendMessage(s_hwndServer, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->useD3D11 = (SendMessage(s_hwndUseD3D11, BM_GETCHECK, 0, 0) == BST_CHECKED);

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
                    s_currentSettings->port = port;
                    s_currentSettings->inputConfig.boundaryX = boundaryX;

                    PostQuitMessage(1);
                } catch (const std::exception&) {
                    MessageBox(hwnd, "Please enter valid numeric values for Port and Boundary.", "Input Error", MB_OK | MB_ICONERROR);
                }
            }
            break;

        case WM_CLOSE:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

static std::map<unsigned long long, PeerState> s_cachedPeers;
static std::mutex s_monitorMutex;

void ConfigGUI::UpdateCursorMonitor(const std::map<unsigned long long, PeerState>& peers) {
#ifdef _WIN32
    if (s_hwndCursorMonitor) {
        {
            std::lock_guard<std::mutex> lock(s_monitorMutex);
            s_cachedPeers = peers;
        }

        RECT rect;
        GetClientRect(s_hwndCursorMonitor, &rect);
        HDC hdc = GetDC(s_hwndCursorMonitor);

        // Background
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        // Draw minimized visualization of cursors
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
    std::cout << "[GUI] Launching configuration dialog..." << std::endl;

#ifdef _WIN32
    s_currentSettings = &settings;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "NetMuxSettings";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("NetMuxSettings", "NetMux Settings", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 300, 400, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;

    NetworkManager discoveryNetwork;

    MSG msg;
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Poll for discovery or update peer list from SyncModule
            if (sync) {
                auto peers = sync->GetAllPeers();
                for (auto const& [id, peer] : peers) {
                    std::string entry = std::string(peer.sessionName) + " (Group " + std::to_string(peer.groupId) + ")";
                    if (SendMessage(s_hwndPeerList, CB_FINDSTRINGEXACT, -1, (LPARAM)entry.c_str()) == CB_ERR) {
                        SendMessage(s_hwndPeerList, CB_ADDSTRING, 0, (LPARAM)entry.c_str());
                    }
                }
            } else {
                DiscoveryPacket dp;
                if (discoveryNetwork.PollDiscovery(dp)) {
                    if (SendMessage(s_hwndPeerList, CB_FINDSTRINGEXACT, -1, (LPARAM)dp.hostname) == CB_ERR) {
                        SendMessage(s_hwndPeerList, CB_ADDSTRING, 0, (LPARAM)dp.hostname);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    return (msg.wParam == 1);
#else
    return true;
#endif
}
