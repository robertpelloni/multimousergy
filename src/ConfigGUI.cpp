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
static FileTransferEngine* s_currentFileTransfer = nullptr;
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
static HWND s_hwndSelColorR = nullptr;
static HWND s_hwndSelColorG = nullptr;
static HWND s_hwndSelColorB = nullptr;
static HWND s_hwndCursorMonitor = nullptr;
static HWND s_hwndSecurityLog = nullptr;
static HWND s_hwndDiscoveryList = nullptr;

enum ControlIDs {
    ID_SAVE_BUTTON = 1,
    ID_DISCONNECT_BUTTON = 22,
    ID_IP_ADDRESS = 19,
    ID_PORT = 20,
    ID_BOUNDARY = 21,
    ID_PEER_LIST = 2,
    ID_CURSOR_SCALE = 3,
    ID_USE_D3D11 = 4,
    ID_GROUP_ID = 5,
    ID_SESSION_NAME = 6,
    ID_GROUP_NAME = 7,
    ID_SECURITY_KEY = 8,
    ID_CURSOR_THEME_PATH = 14,
    ID_BROWSE_THEME = 15,
    ID_SEL_COLOR_R = 16,
    ID_SEL_COLOR_G = 17,
    ID_SEL_COLOR_B = 18,
    ID_CURSOR_MONITOR = 9,
    ID_DRIVER_TYPE = 10,
    ID_SECURITY_LOG = 11,
    ID_SCAN_BUTTON = 12,
    ID_DISCOVERY_LIST = 13,
    ID_SEND_FILE_BUTTON = 23,
    ID_FILE_TRANSFER_LIST = 24,
    ID_RECENT_SERVERS = 25,
    ID_AUTO_CONNECT = 26,
    ID_CLEAR_HISTORY = 27,
    ID_PEER_COLOR_R = 28,
    ID_PEER_COLOR_G = 29,
    ID_PEER_COLOR_B = 30
};

static HWND s_hwndFileTransferList = nullptr;
static HWND s_hwndRecentServers = nullptr;
static HWND s_hwndAutoConnect = nullptr;
static HWND s_hwndPeerColorR = nullptr;
static HWND s_hwndPeerColorG = nullptr;
static HWND s_hwndPeerColorB = nullptr;

static HWND s_hwndSecurityStatus = nullptr;

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // Connection Group
            CreateWindow("BUTTON", "Connection Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 5, 5, 345, 335, hwnd, NULL, NULL, NULL);

            char host[64], local_ip[64];
            strcpy(local_ip, "127.0.0.1");
            if (gethostname(host, 64) == 0) {
                struct hostent* h = gethostbyname(host);
                if (h) strcpy(local_ip, inet_ntoa(*(struct in_addr*)h->h_addr));
            }
            char info[128]; snprintf(info, 128, "Local: %s (%s)", host, local_ip);
            CreateWindow("STATIC", info, WS_VISIBLE | WS_CHILD, 15, 25, 320, 20, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Role:", WS_VISIBLE | WS_CHILD, 15, 50, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndServer = CreateWindow("BUTTON", "Server Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 115, 50, 150, 20, hwnd, NULL, NULL, NULL);
            if (s_currentSettings->isServer) SendMessage(s_hwndServer, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindow("STATIC", "Remote IP:", WS_VISIBLE | WS_CHILD, 15, 75, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndIp = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", s_currentSettings->remoteIp.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 115, 75, 150, 20, hwnd, (HMENU)ID_IP_ADDRESS, NULL, NULL);

            CreateWindow("STATIC", "Port:", WS_VISIBLE | WS_CHILD, 15, 100, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndPort = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", std::to_string(s_currentSettings->port).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER, 115, 100, 150, 20, hwnd, (HMENU)ID_PORT, NULL, NULL);

            CreateWindow("STATIC", "Boundary X:", WS_VISIBLE | WS_CHILD, 15, 125, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndBoundary = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", std::to_string(s_currentSettings->inputConfig.boundaryX).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER, 115, 125, 150, 20, hwnd, (HMENU)ID_BOUNDARY, NULL, NULL);

            s_hwndAutoConnect = CreateWindow("BUTTON", "Auto-Reconnect", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 15, 150, 250, 20, hwnd, (HMENU)ID_AUTO_CONNECT, NULL, NULL);
            if (s_currentSettings->autoConnect) SendMessage(s_hwndAutoConnect, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindow("BUTTON", "Apply & Connect", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 15, 175, 120, 30, hwnd, (HMENU)ID_SAVE_BUTTON, NULL, NULL);
            CreateWindow("BUTTON", "Disconnect", WS_VISIBLE | WS_CHILD, 145, 175, 120, 30, hwnd, (HMENU)ID_DISCONNECT_BUTTON, NULL, NULL);

            CreateWindow("STATIC", "History:", WS_VISIBLE | WS_CHILD, 15, 210, 60, 20, hwnd, NULL, NULL, NULL);
            s_hwndRecentServers = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 80, 210, 240, 100, hwnd, (HMENU)ID_RECENT_SERVERS, NULL, NULL);
            for (auto const& s : s_currentSettings->recentServers) SendMessage(s_hwndRecentServers, CB_ADDSTRING, 0, (LPARAM)s.c_str());

            CreateWindow("BUTTON", "Scan", WS_VISIBLE | WS_CHILD, 15, 235, 60, 25, hwnd, (HMENU)ID_SCAN_BUTTON, NULL, NULL);
            CreateWindow("BUTTON", "Clear", WS_VISIBLE | WS_CHILD, 80, 235, 60, 25, hwnd, (HMENU)ID_CLEAR_HISTORY, NULL, NULL);
            s_hwndDiscoveryList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER | LBS_NOTIFY, 15, 265, 305, 60, hwnd, (HMENU)ID_DISCOVERY_LIST, NULL, NULL);

            // Session Group
            CreateWindow("BUTTON", "Session & Security", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 5, 345, 345, 145, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Session:", WS_VISIBLE | WS_CHILD, 15, 365, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSessionName = CreateWindow("EDIT", s_currentSettings->sessionName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 115, 365, 150, 20, hwnd, (HMENU)ID_SESSION_NAME, NULL, NULL);

            CreateWindow("STATIC", "Group ID:", WS_VISIBLE | WS_CHILD, 15, 390, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndGroupId = CreateWindow("EDIT", std::to_string(s_currentSettings->groupId).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 115, 390, 150, 20, hwnd, (HMENU)ID_GROUP_ID, NULL, NULL);

            CreateWindow("STATIC", "Group Name:", WS_VISIBLE | WS_CHILD, 15, 415, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndGroupName = CreateWindow("EDIT", s_currentSettings->groupName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 115, 415, 150, 20, hwnd, (HMENU)ID_GROUP_NAME, NULL, NULL);

            CreateWindow("STATIC", "Security Key:", WS_VISIBLE | WS_CHILD, 15, 440, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSecurityKey = CreateWindow("EDIT", s_currentSettings->securityKey.c_str(), WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_BORDER, 115, 440, 150, 20, hwnd, (HMENU)ID_SECURITY_KEY, NULL, NULL);

            s_hwndSecurityStatus = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD, 15, 465, 320, 20, hwnd, NULL, NULL, NULL);
            if (s_currentSettings->securityKey.empty()) SetWindowText(s_hwndSecurityStatus, "Status: UNSECURED | IDLE");
            else SetWindowText(s_hwndSecurityStatus, "Status: ENCRYPTED | IDLE");

            // Cursor Group
            CreateWindow("BUTTON", "Cursor Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 5, 495, 345, 165, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Peer Color:", WS_VISIBLE | WS_CHILD, 15, 515, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndPeerColorR = CreateWindow("EDIT", std::to_string(s_currentSettings->peerColorR).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 115, 515, 35, 20, hwnd, (HMENU)ID_PEER_COLOR_R, NULL, NULL);
            s_hwndPeerColorG = CreateWindow("EDIT", std::to_string(s_currentSettings->peerColorG).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 155, 515, 35, 20, hwnd, (HMENU)ID_PEER_COLOR_G, NULL, NULL);
            s_hwndPeerColorB = CreateWindow("EDIT", std::to_string(s_currentSettings->peerColorB).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 195, 515, 35, 20, hwnd, (HMENU)ID_PEER_COLOR_B, NULL, NULL);

            CreateWindow("STATIC", "Driver:", WS_VISIBLE | WS_CHILD, 15, 540, 60, 20, hwnd, NULL, NULL, NULL);
            s_hwndNetMuxDriverType = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 115, 540, 150, 100, hwnd, (HMENU)ID_DRIVER_TYPE, NULL, NULL);
            SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"Auto");
            SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"Interception");
            SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"ViGEmBus");
            SendMessage(s_hwndNetMuxDriverType, CB_SETCURSEL, (WPARAM)s_currentSettings->driverType, 0);

            CreateWindow("STATIC", "Scale (%):", WS_VISIBLE | WS_CHILD, 15, 565, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorScale = CreateWindow("EDIT", "100", WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 115, 565, 50, 20, hwnd, (HMENU)ID_CURSOR_SCALE, NULL, NULL);

            s_hwndUseD3D11 = CreateWindow("BUTTON", "D3D11 Acceleration", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 15, 590, 250, 20, hwnd, (HMENU)ID_USE_D3D11, NULL, NULL);
            if (s_currentSettings->useD3D11) SendMessage(s_hwndUseD3D11, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindow("STATIC", "Theme:", WS_VISIBLE | WS_CHILD, 15, 610, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndCursorThemePath = CreateWindow("EDIT", s_currentSettings->cursorThemePath.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 115, 610, 120, 20, hwnd, (HMENU)ID_CURSOR_THEME_PATH, NULL, NULL);
            CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 240, 610, 25, 20, hwnd, (HMENU)ID_BROWSE_THEME, NULL, NULL);

            CreateWindow("STATIC", "Sel RGB:", WS_VISIBLE | WS_CHILD, 15, 635, 100, 20, hwnd, NULL, NULL, NULL);
            s_hwndSelColorR = CreateWindow("EDIT", std::to_string(s_currentSettings->selectionColorR).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 115, 635, 35, 20, hwnd, (HMENU)ID_SEL_COLOR_R, NULL, NULL);
            s_hwndSelColorG = CreateWindow("EDIT", std::to_string(s_currentSettings->selectionColorG).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 155, 635, 35, 20, hwnd, (HMENU)ID_SEL_COLOR_G, NULL, NULL);
            s_hwndSelColorB = CreateWindow("EDIT", std::to_string(s_currentSettings->selectionColorB).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 195, 635, 35, 20, hwnd, (HMENU)ID_SEL_COLOR_B, NULL, NULL);

            // Metrics Group
            CreateWindow("BUTTON", "Real-Time Monitor", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 5, 665, 345, 330, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Peer List & Metrics:", WS_VISIBLE | WS_CHILD, 15, 685, 250, 20, hwnd, NULL, NULL, NULL);
            s_hwndPeerList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 15, 705, 315, 70, hwnd, (HMENU)ID_PEER_LIST, NULL, NULL);

            s_hwndCursorMonitor = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | SS_NOTIFY | WS_BORDER, 15, 780, 315, 80, hwnd, (HMENU)ID_CURSOR_MONITOR, NULL, NULL);

            CreateWindow("STATIC", "Security Log:", WS_VISIBLE | WS_CHILD, 15, 865, 150, 20, hwnd, NULL, NULL, NULL);
            s_hwndSecurityLog = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 15, 885, 315, 40, hwnd, (HMENU)ID_SECURITY_LOG, NULL, NULL);

            CreateWindow("STATIC", "File Transfers:", WS_VISIBLE | WS_CHILD, 15, 930, 150, 20, hwnd, NULL, NULL, NULL);
            s_hwndFileTransferList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 15, 950, 315, 40, hwnd, (HMENU)ID_FILE_TRANSFER_LIST, NULL, NULL);
            CreateWindow("BUTTON", "Send File...", WS_VISIBLE | WS_CHILD, 15, 995, 120, 25, hwnd, (HMENU)ID_SEND_FILE_BUTTON, NULL, NULL);
            break;

        case WM_DRAWITEM:
            if (wParam == ID_CURSOR_MONITOR) {
                LPDRAWITEMSTRUCT pdi = (LPDRAWITEMSTRUCT)lParam;
                FillRect(pdi->hDC, &pdi->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BROWSE_THEME) {
                OPENFILENAMEA ofn;
                char szFile[260] = {0};
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "Bitmaps (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    SetWindowText(s_hwndCursorThemePath, ofn.lpstrFile);
                }
            }

            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == ID_RECENT_SERVERS) {
                int index = (int)SendMessage(s_hwndRecentServers, CB_GETCURSEL, 0, 0);
                if (index != CB_ERR) {
                    char buffer[256];
                    SendMessage(s_hwndRecentServers, CB_GETLBTEXT, index, (LPARAM)buffer);
                    SetWindowText(s_hwndIp, buffer);
                }
            }

            if (LOWORD(wParam) == ID_CLEAR_HISTORY) {
                s_currentSettings->recentServers.clear();
                SendMessage(s_hwndRecentServers, CB_RESETCONTENT, 0, 0);
                ConfigGUI::LogSecurityEvent("Connection history cleared.");
            }

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

            if (LOWORD(wParam) == ID_DISCONNECT_BUTTON) {
                s_restartRequested = true;
                // We reuse the restart mechanism to force a stop/re-init cycle but we can also set a flag
                // to not auto-restart if we wanted. For now, it just stops the framework.
                s_isRunning = false;
                PostMessage(hwnd, WM_USER + 1, 0, 0);
            }

            if (LOWORD(wParam) == ID_SEND_FILE_BUTTON) {
                if (s_currentFileTransfer) {
                    OPENFILENAMEA ofn;
                    char szFile[260] = {0};
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameA(&ofn)) {
                        s_currentFileTransfer->StartTransfer(ofn.lpstrFile, 0); // Send to all for now
                        ConfigGUI::LogSecurityEvent("Started file transfer: " + std::string(ofn.lpstrFile));
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
                    s_currentSettings->autoConnect = (SendMessage(s_hwndAutoConnect, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->useD3D11 = (SendMessage(s_hwndUseD3D11, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->driverType = (NetMuxDriverType)SendMessage(s_hwndNetMuxDriverType, CB_GETCURSEL, 0, 0);

                    GetWindowText(s_hwndPeerColorR, buffer, 256);
                    s_currentSettings->peerColorR = (unsigned char)std::stoi(buffer);
                    GetWindowText(s_hwndPeerColorG, buffer, 256);
                    s_currentSettings->peerColorG = (unsigned char)std::stoi(buffer);
                    GetWindowText(s_hwndPeerColorB, buffer, 256);
                    s_currentSettings->peerColorB = (unsigned char)std::stoi(buffer);

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
                    if (!s_currentSettings->isServer && !s_currentSettings->remoteIp.empty()) {
                        bool exists = false;
                        for (auto const& rs : s_currentSettings->recentServers) if (rs == s_currentSettings->remoteIp) { exists = true; break; }
                        if (!exists) {
                            s_currentSettings->recentServers.insert(s_currentSettings->recentServers.begin(), s_currentSettings->remoteIp);
                            if (s_currentSettings->recentServers.size() > 5) s_currentSettings->recentServers.pop_back();
                        }
                    }

                    GetWindowText(s_hwndCursorThemePath, buffer, 256);
                    s_currentSettings->cursorThemePath = buffer;

                    GetWindowText(s_hwndSelColorR, buffer, 256);
                    s_currentSettings->selectionColorR = (unsigned char)std::stoi(buffer);
                    GetWindowText(s_hwndSelColorG, buffer, 256);
                    s_currentSettings->selectionColorG = (unsigned char)std::stoi(buffer);
                    GetWindowText(s_hwndSelColorB, buffer, 256);
                    s_currentSettings->selectionColorB = (unsigned char)std::stoi(buffer);

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

void ConfigGUI::Initialize(AppSettings& settings, SyncModule* sync, NetworkManager* network, FileTransferEngine* fileTransfer) {
    s_currentSettings = &settings;
    s_currentSync = sync;
    s_currentNetwork = network;
    s_currentFileTransfer = fileTransfer;
    s_restartRequested = false;

#ifdef _WIN32
    if (s_hwndMain && IsWindow(s_hwndMain)) {
        ShowWindow(s_hwndMain, SW_SHOW);
        SetForegroundWindow(s_hwndMain);
        s_isRunning = true;
        return;
    }

    WNDCLASS wc = {0};
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "NetMuxIntegratedGUI";
    RegisterClass(&wc);

    s_hwndMain = CreateWindow("NetMuxIntegratedGUI", "NetMux Monitor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 370, 1070, NULL, NULL, GetModuleHandle(NULL), NULL);
    s_isRunning = (s_hwndMain != NULL);
#else
    s_isRunning = true;
#endif
}

void ConfigGUI::SetSyncModule(SyncModule* sync) {
    s_currentSync = sync;
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

    if (s_isRunning && s_currentFileTransfer) {
        static double lastFileUpdate = 0;
        static std::map<unsigned long long, std::string> lastFileStates;
        double now = GetTickCount64();
        if (now - lastFileUpdate > 500.0) {
            auto transfers = s_currentFileTransfer->GetActiveTransfers();
            bool changed = false;
            if (transfers.size() != lastFileStates.size()) changed = true;
            else {
                for (auto const& [id, t] : transfers) {
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%s | %.1f%% | %s", t.filename.c_str(), t.progress * 100.0f, t.isOutgoing ? "OUT" : "IN");
                    if (lastFileStates[id] != buffer) { changed = true; break; }
                }
            }

            if (changed) {
                SendMessage(s_hwndFileTransferList, LB_RESETCONTENT, 0, 0);
                lastFileStates.clear();
                for (auto const& [id, t] : transfers) {
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%s | %.1f%% | %s", t.filename.c_str(), t.progress * 100.0f, t.isOutgoing ? "OUT" : "IN");
                    SendMessage(s_hwndFileTransferList, LB_ADDSTRING, 0, (LPARAM)buffer);
                    lastFileStates[id] = buffer;
                }
            }
            lastFileUpdate = now;
        }
    }

    if (s_isRunning && s_currentNetwork) {
        static ConnectionState lastState = ConnectionState::Disconnected;
        ConnectionState currentState = s_currentNetwork->GetTcpState();
        if (currentState != lastState) {
            const char* stateStr = "IDLE";
            if (currentState == ConnectionState::Connecting) stateStr = "CONNECTING";
            else if (currentState == ConnectionState::Connected) stateStr = "CONNECTED";
            else if (currentState == ConnectionState::Error) stateStr = "ERROR";

            char status[256];
            if (s_currentSettings->securityKey.empty()) snprintf(status, sizeof(status), "Status: UNSECURED | %s", stateStr);
            else snprintf(status, sizeof(status), "Status: ENCRYPTED | %s", stateStr);
            SetWindowText(s_hwndSecurityStatus, status);

            ConfigGUI::LogSecurityEvent("Network state changed to: " + std::string(stateStr));
            lastState = currentState;
        }
    }

    if (s_isRunning && s_currentSync) {
        static double lastPeerUpdate = 0;
        static std::map<unsigned long long, std::string> lastPeerStates;
        double now = GetTickCount64();
        if (now - lastPeerUpdate > 200.0) {
            auto peers = s_currentSync->GetAllPeers();
            bool changed = false;
            if (peers.size() != lastPeerStates.size()) changed = true;
            else {
                for (auto const& [id, peer] : peers) {
                    char info[256];
                    char btnStr[16] = "---";
                    if (peer.buttonState & 1) btnStr[0] = 'L';
                    if (peer.buttonState & 2) btnStr[1] = 'R';
                    if (peer.buttonState & 4) btnStr[2] = 'M';
                    const char* authStatus = (id == s_currentSync->GetLocalId() || s_currentSettings->securityKey.empty()) ? "OPEN" : (peer.isAuthenticated ? "AUTH" : "LOCKED");
                    snprintf(info, sizeof(info), "%s | RTT:%dms | Drift:%.1fpx | [%s] | %s", peer.sessionName, (int)peer.latency, peer.drift, authStatus, btnStr);
                    if (lastPeerStates[id] != info) { changed = true; break; }
                }
            }

            if (changed) {
                SendMessage(s_hwndPeerList, LB_RESETCONTENT, 0, 0);
                lastPeerStates.clear();
                for (auto const& [id, peer] : peers) {
                    char info[256];
                    char btnStr[16] = "---";
                    if (peer.buttonState & 1) btnStr[0] = 'L';
                    if (peer.buttonState & 2) btnStr[1] = 'R';
                    if (peer.buttonState & 4) btnStr[2] = 'M';
                    const char* authStatus = (id == s_currentSync->GetLocalId() || s_currentSettings->securityKey.empty()) ? "OPEN" : (peer.isAuthenticated ? "AUTH" : "LOCKED");
                    snprintf(info, sizeof(info), "%s | RTT:%dms | Drift:%.1fpx | [%s] | %s", peer.sessionName, (int)peer.latency, peer.drift, authStatus, btnStr);
                    SendMessage(s_hwndPeerList, LB_ADDSTRING, 0, (LPARAM)info);
                    lastPeerStates[id] = info;
                }
            }
            lastPeerUpdate = now;
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
    if (s_hwndMain && IsWindow(s_hwndMain)) {
        DestroyWindow(s_hwndMain);
    }
    s_hwndMain = nullptr;
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
            // Show everyone on the minimap for debugging and coordination
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
    Initialize(settings, sync, nullptr, nullptr);

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
