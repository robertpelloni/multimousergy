#include "ConfigGUI.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <mutex>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define WM_TRAYICON (WM_USER + 100)
#define ID_TRAY_EXIT 2001
#define ID_TRAY_SHOW 2002
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
    ID_PEER_COLOR_B = 30,
    ID_TAB_CONTROL = 31,
    ID_DIAGNOSTICS_LIST = 32,
    ID_QUIT_BUTTON = 33,
    ID_DISPLAY_NAME = 34,
    ID_START_MINIMIZED = 35
};

static HWND s_hwndFileTransferList = nullptr;
static HWND s_hwndTabControl = nullptr;
static HWND s_hwndRecentServers = nullptr;
static HWND s_hwndAutoConnect = nullptr;
static HWND s_hwndStartMinimized = nullptr;
static HWND s_hwndPeerColorR = nullptr;
static HWND s_hwndPeerColorG = nullptr;
static HWND s_hwndPeerColorB = nullptr;
static HWND s_hwndDisplayName = nullptr;

static HWND s_hwndSecurityStatus = nullptr;

static std::vector<HWND> s_groupHWNDs[5];

static HWND CreateToolTip(HWND hwndParent, HWND hwndControl, const char* text) {
    if (!hwndControl || !text) return NULL;
    HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 hwndParent, NULL, GetModuleHandle(NULL), NULL);
    if (!hwndTip) return NULL;

    TOOLINFO toolInfo = {0};
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hwndParent;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwndControl;
    toolInfo.lpszText = (char*)text;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    return hwndTip;
}

static void ShowGroup(int tabIndex) {
    for (int i = 0; i < 5; ++i) {
        int cmd = (i == tabIndex) ? SW_SHOW : SW_HIDE;
        for (HWND h : s_groupHWNDs[i]) ShowWindow(h, cmd);
    }
}

static void CreateConnectionTab(HWND hwnd) {
    s_groupHWNDs[0].push_back(CreateWindow("BUTTON", "Connection Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 35, 325, 335, hwnd, NULL, NULL, NULL));

    char host[64], local_ip[64];
    strcpy(local_ip, "127.0.0.1");
    if (gethostname(host, 64) == 0) {
        struct hostent* h = gethostbyname(host);
        if (h) strcpy(local_ip, inet_ntoa(*(struct in_addr*)h->h_addr));
    }
    char info[128]; snprintf(info, 128, "Local: %s (%s)", host, local_ip);
    s_groupHWNDs[0].push_back(CreateWindow("STATIC", info, WS_VISIBLE | WS_CHILD, 20, 60, 310, 20, hwnd, NULL, NULL, NULL));

    s_groupHWNDs[0].push_back(CreateWindow("STATIC", "Role:", WS_VISIBLE | WS_CHILD, 20, 85, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndServer = CreateWindow("BUTTON", "Server Mode", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 120, 85, 150, 20, hwnd, NULL, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndServer);
    CreateToolTip(hwnd, s_hwndServer, "Enable to act as the authoritative host for this cursor group.");
    if (s_currentSettings->isServer) SendMessage(s_hwndServer, BM_SETCHECK, BST_CHECKED, 0);

    s_groupHWNDs[0].push_back(CreateWindow("STATIC", "Remote IP:", WS_VISIBLE | WS_CHILD, 20, 110, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndIp = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", s_currentSettings->remoteIp.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 120, 110, 150, 20, hwnd, (HMENU)ID_IP_ADDRESS, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndIp);

    s_groupHWNDs[0].push_back(CreateWindow("STATIC", "Port:", WS_VISIBLE | WS_CHILD, 20, 135, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndPort = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", std::to_string(s_currentSettings->port).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER, 120, 135, 150, 20, hwnd, (HMENU)ID_PORT, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndPort);

    s_groupHWNDs[0].push_back(CreateWindow("STATIC", "Boundary X:", WS_VISIBLE | WS_CHILD, 20, 160, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndBoundary = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", std::to_string(s_currentSettings->inputConfig.boundaryX).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER, 120, 160, 150, 20, hwnd, (HMENU)ID_BOUNDARY, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndBoundary);

    s_hwndAutoConnect = CreateWindow("BUTTON", "Auto-Reconnect", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 185, 250, 20, hwnd, (HMENU)ID_AUTO_CONNECT, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndAutoConnect);
    if (s_currentSettings->autoConnect) SendMessage(s_hwndAutoConnect, BM_SETCHECK, BST_CHECKED, 0);

    HWND hwndApply = CreateWindow("BUTTON", "Apply", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 210, 100, 30, hwnd, (HMENU)ID_SAVE_BUTTON, NULL, NULL);
    s_groupHWNDs[0].push_back(hwndApply);
    CreateToolTip(hwnd, hwndApply, "Saves current settings and attempts to connect or start the server.");

    HWND hwndDisc = CreateWindow("BUTTON", "Disconnect", WS_VISIBLE | WS_CHILD, 125, 210, 100, 30, hwnd, (HMENU)ID_DISCONNECT_BUTTON, NULL, NULL);
    s_groupHWNDs[0].push_back(hwndDisc);
    CreateToolTip(hwnd, hwndDisc, "Gracefully disconnects from all peers and stops the network engine.");

    HWND hwndQuit = CreateWindow("BUTTON", "Quit", WS_VISIBLE | WS_CHILD, 230, 210, 100, 30, hwnd, (HMENU)ID_QUIT_BUTTON, NULL, NULL);
    s_groupHWNDs[0].push_back(hwndQuit);
    CreateToolTip(hwnd, hwndQuit, "Exits the application completely.");

    s_groupHWNDs[0].push_back(CreateWindow("STATIC", "History:", WS_VISIBLE | WS_CHILD, 20, 245, 60, 20, hwnd, NULL, NULL, NULL));
    s_hwndRecentServers = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 85, 245, 240, 100, hwnd, (HMENU)ID_RECENT_SERVERS, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndRecentServers);
    CreateToolTip(hwnd, s_hwndRecentServers, "A list of recently connected server IP addresses.");
    for (auto const& s : s_currentSettings->recentServers) SendMessage(s_hwndRecentServers, CB_ADDSTRING, 0, (LPARAM)s.c_str());

    s_groupHWNDs[0].push_back(CreateWindow("BUTTON", "Scan", WS_VISIBLE | WS_CHILD, 20, 270, 60, 25, hwnd, (HMENU)ID_SCAN_BUTTON, NULL, NULL));
    s_groupHWNDs[0].push_back(CreateWindow("BUTTON", "Clear", WS_VISIBLE | WS_CHILD, 85, 270, 60, 25, hwnd, (HMENU)ID_CLEAR_HISTORY, NULL, NULL));
    s_hwndDiscoveryList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER | LBS_NOTIFY, 20, 300, 305, 30, hwnd, (HMENU)ID_DISCOVERY_LIST, NULL, NULL);
    s_groupHWNDs[0].push_back(s_hwndDiscoveryList);
    CreateToolTip(hwnd, s_hwndDiscoveryList, "Servers found during network scan. Double-click to auto-fill Remote IP.");
}

static void CreateCursorFileTab(HWND hwnd) {
    // Session Group
    s_groupHWNDs[1].push_back(CreateWindow("BUTTON", "Session & Security", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 35, 325, 145, hwnd, NULL, NULL, NULL));

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Session:", WS_VISIBLE | WS_CHILD, 20, 55, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndSessionName = CreateWindow("EDIT", s_currentSettings->sessionName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 120, 55, 150, 20, hwnd, (HMENU)ID_SESSION_NAME, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndSessionName);

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Group ID:", WS_VISIBLE | WS_CHILD, 20, 80, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndGroupId = CreateWindow("EDIT", std::to_string(s_currentSettings->groupId).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 120, 80, 150, 20, hwnd, (HMENU)ID_GROUP_ID, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndGroupId);
    CreateToolTip(hwnd, s_hwndGroupId, "Peers must have the same Group ID to see and interact with each other.");

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Group Name:", WS_VISIBLE | WS_CHILD, 20, 105, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndGroupName = CreateWindow("EDIT", s_currentSettings->groupName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 120, 105, 150, 20, hwnd, (HMENU)ID_GROUP_NAME, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndGroupName);

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Security Key:", WS_VISIBLE | WS_CHILD, 20, 130, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndSecurityKey = CreateWindow("EDIT", s_currentSettings->securityKey.c_str(), WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL | WS_BORDER, 120, 130, 150, 20, hwnd, (HMENU)ID_SECURITY_KEY, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndSecurityKey);

    s_hwndSecurityStatus = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD, 20, 155, 310, 20, hwnd, NULL, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndSecurityStatus);
    if (s_currentSettings->securityKey.empty()) SetWindowText(s_hwndSecurityStatus, "Status: UNSECURED | IDLE");
    else SetWindowText(s_hwndSecurityStatus, "Status: ENCRYPTED | IDLE");

    // Cursor Group
    s_groupHWNDs[1].push_back(CreateWindow("BUTTON", "Cursor Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 185, 325, 230, hwnd, NULL, NULL, NULL));

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Peer Color:", WS_VISIBLE | WS_CHILD, 20, 205, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndPeerColorR = CreateWindow("EDIT", std::to_string(s_currentSettings->peerColorR).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 120, 205, 35, 20, hwnd, (HMENU)ID_PEER_COLOR_R, NULL, NULL);
    s_hwndPeerColorG = CreateWindow("EDIT", std::to_string(s_currentSettings->peerColorG).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 160, 205, 35, 20, hwnd, (HMENU)ID_PEER_COLOR_G, NULL, NULL);
    s_hwndPeerColorB = CreateWindow("EDIT", std::to_string(s_currentSettings->peerColorB).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 200, 205, 35, 20, hwnd, (HMENU)ID_PEER_COLOR_B, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndPeerColorR); s_groupHWNDs[1].push_back(s_hwndPeerColorG); s_groupHWNDs[1].push_back(s_hwndPeerColorB);

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Driver:", WS_VISIBLE | WS_CHILD, 20, 230, 60, 20, hwnd, NULL, NULL, NULL));
    s_hwndNetMuxDriverType = CreateWindow("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 120, 230, 150, 100, hwnd, (HMENU)ID_DRIVER_TYPE, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndNetMuxDriverType);
    SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)"Auto");

    std::string intStr = "Interception" + std::string(DriverInterface::IsDriverInstalled(NetMuxDriverType::Interception) ? " [Found]" : " [Not Found]");
    SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)intStr.c_str());

    std::string vigStr = "ViGEmBus" + std::string(DriverInterface::IsDriverInstalled(NetMuxDriverType::ViGEmBus) ? " [Found]" : " [Not Found]");
    SendMessage(s_hwndNetMuxDriverType, CB_ADDSTRING, 0, (LPARAM)vigStr.c_str());
    SendMessage(s_hwndNetMuxDriverType, CB_SETCURSEL, (WPARAM)s_currentSettings->driverType, 0);

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Scale (%):", WS_VISIBLE | WS_CHILD, 20, 255, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndCursorScale = CreateWindow("EDIT", "100", WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 120, 255, 50, 20, hwnd, (HMENU)ID_CURSOR_SCALE, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndCursorScale);

    s_hwndUseD3D11 = CreateWindow("BUTTON", "D3D11 Acceleration", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 280, 250, 20, hwnd, (HMENU)ID_USE_D3D11, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndUseD3D11);
    if (s_currentSettings->useD3D11) SendMessage(s_hwndUseD3D11, BM_SETCHECK, BST_CHECKED, 0);

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Theme:", WS_VISIBLE | WS_CHILD, 20, 305, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndCursorThemePath = CreateWindow("EDIT", s_currentSettings->cursorThemePath.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER, 120, 305, 120, 20, hwnd, (HMENU)ID_CURSOR_THEME_PATH, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndCursorThemePath);
    s_groupHWNDs[1].push_back(CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD, 245, 305, 25, 20, hwnd, (HMENU)ID_BROWSE_THEME, NULL, NULL));

    s_groupHWNDs[1].push_back(CreateWindow("STATIC", "Sel RGB:", WS_VISIBLE | WS_CHILD, 20, 330, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndSelColorR = CreateWindow("EDIT", std::to_string(s_currentSettings->selectionColorR).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 120, 330, 35, 20, hwnd, (HMENU)ID_SEL_COLOR_R, NULL, NULL);
    s_hwndSelColorG = CreateWindow("EDIT", std::to_string(s_currentSettings->selectionColorG).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 160, 330, 35, 20, hwnd, (HMENU)ID_SEL_COLOR_G, NULL, NULL);
    s_hwndSelColorB = CreateWindow("EDIT", std::to_string(s_currentSettings->selectionColorB).c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | WS_BORDER, 200, 330, 35, 20, hwnd, (HMENU)ID_SEL_COLOR_B, NULL, NULL);
    s_groupHWNDs[1].push_back(s_hwndSelColorR); s_groupHWNDs[1].push_back(s_hwndSelColorG); s_groupHWNDs[1].push_back(s_hwndSelColorB);
}

static void CreateMonitorTab(HWND hwnd) {
    // Metrics Group
    s_groupHWNDs[2].push_back(CreateWindow("BUTTON", "Real-Time Monitor", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 35, 325, 330, hwnd, NULL, NULL, NULL));

    s_groupHWNDs[2].push_back(CreateWindow("STATIC", "Peer List & Metrics:", WS_VISIBLE | WS_CHILD, 20, 55, 250, 20, hwnd, NULL, NULL, NULL));
    s_hwndPeerList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 20, 75, 305, 70, hwnd, (HMENU)ID_PEER_LIST, NULL, NULL);
    s_groupHWNDs[2].push_back(s_hwndPeerList);

    s_hwndCursorMonitor = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | SS_NOTIFY | WS_BORDER, 20, 150, 305, 80, hwnd, (HMENU)ID_CURSOR_MONITOR, NULL, NULL);
    s_groupHWNDs[2].push_back(s_hwndCursorMonitor);
    SetWindowSubclass(s_hwndCursorMonitor, MonitorSubclassProc, 0, 0);

    s_groupHWNDs[2].push_back(CreateWindow("STATIC", "Security Log:", WS_VISIBLE | WS_CHILD, 20, 235, 150, 20, hwnd, NULL, NULL, NULL));
    s_hwndSecurityLog = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 20, 255, 305, 40, hwnd, (HMENU)ID_SECURITY_LOG, NULL, NULL);
    s_groupHWNDs[2].push_back(s_hwndSecurityLog);

    s_groupHWNDs[2].push_back(CreateWindow("STATIC", "File Transfers:", WS_VISIBLE | WS_CHILD, 20, 300, 150, 20, hwnd, NULL, NULL, NULL));
    s_hwndFileTransferList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 20, 320, 305, 40, hwnd, (HMENU)ID_FILE_TRANSFER_LIST, NULL, NULL);
    s_groupHWNDs[2].push_back(s_hwndFileTransferList);
    HWND hwndSendFile = CreateWindow("BUTTON", "Send File...", WS_VISIBLE | WS_CHILD, 20, 365, 120, 25, hwnd, (HMENU)ID_SEND_FILE_BUTTON, NULL, NULL);
    s_groupHWNDs[2].push_back(hwndSendFile);
    CreateToolTip(hwnd, hwndSendFile, "Select and broadcast a file to all connected peers in your group.");
}

static void CreateLogsTab(HWND hwnd) {
    // Logs Group
    s_groupHWNDs[3].push_back(CreateWindow("BUTTON", "Diagnostics Log", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 35, 325, 330, hwnd, NULL, NULL, NULL));
    s_hwndDiagnosticsList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_BORDER, 20, 55, 305, 260, hwnd, (HMENU)ID_DIAGNOSTICS_LIST, NULL, NULL);
    s_groupHWNDs[3].push_back(s_hwndDiagnosticsList);
}

static void CreateSettingsTab(HWND hwnd) {
    s_groupHWNDs[4].push_back(CreateWindow("BUTTON", "Global Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 35, 325, 145, hwnd, NULL, NULL, NULL));

    s_groupHWNDs[4].push_back(CreateWindow("STATIC", "Display Name:", WS_VISIBLE | WS_CHILD, 20, 55, 100, 20, hwnd, NULL, NULL, NULL));
    s_hwndDisplayName = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", s_currentSettings->displayName.c_str(), WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 120, 55, 150, 20, hwnd, (HMENU)ID_DISPLAY_NAME, NULL, NULL);
    s_groupHWNDs[4].push_back(s_hwndDisplayName);
    CreateToolTip(hwnd, s_hwndDisplayName, "Friendly name shown to other peers on the network.");

    s_hwndStartMinimized = CreateWindow("BUTTON", "Start Minimized to Tray", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 85, 250, 20, hwnd, (HMENU)ID_START_MINIMIZED, NULL, NULL);
    s_groupHWNDs[4].push_back(s_hwndStartMinimized);
    if (s_currentSettings->startMinimized) SendMessage(s_hwndStartMinimized, BM_SETCHECK, BST_CHECKED, 0);
    CreateToolTip(hwnd, s_hwndStartMinimized, "If enabled, the monitor window will automatically hide to the system tray on startup.");
}

LRESULT CALLBACK MonitorSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_LBUTTONDOWN) {
        if (s_currentSync) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);

            auto peers = s_currentSync->GetAllPeers();
            for (auto const& [id, peer] : peers) {
                int px = (int)((peer.normalizedX * (rect.right - 10)) / 65535) + 5;
                int py = (int)((peer.normalizedY * (rect.bottom - 10)) / 65535) + 5;
                if (abs(mx - px) < 10 && abs(my - py) < 10) {
                    s_currentSync->SetActivePeer(id);
                    ConfigGUI::LogSecurityEvent("Manually switched focus to Peer: " + std::to_string(id));
                    break;
                }
            }
        }
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
            } else if (LOWORD(lParam) == WM_RBUTTONUP) {
                POINT pt; GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, "Show Monitor");
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Quit NetMux");
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
            break;

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_MINIMIZE) {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;

        case WM_CREATE:
            s_hwndTabControl = CreateWindowEx(0, WC_TABCONTROL, "", WS_CHILD | WS_VISIBLE, 5, 5, 345, 1020, hwnd, (HMENU)ID_TAB_CONTROL, GetModuleHandle(NULL), NULL);
            TCITEM tie;
            tie.mask = TCIF_TEXT;
            tie.pszText = (char*)"Connection"; TabCtrl_InsertItem(s_hwndTabControl, 0, &tie);
            tie.pszText = (char*)"Cursor & File"; TabCtrl_InsertItem(s_hwndTabControl, 1, &tie);
            tie.pszText = (char*)"Monitor"; TabCtrl_InsertItem(s_hwndTabControl, 2, &tie);
            tie.pszText = (char*)"Logs"; TabCtrl_InsertItem(s_hwndTabControl, 3, &tie);
            tie.pszText = (char*)"Settings"; TabCtrl_InsertItem(s_hwndTabControl, 4, &tie);

            for (int i = 0; i < 5; ++i) s_groupHWNDs[i].clear();

            CreateConnectionTab(hwnd);
            CreateCursorFileTab(hwnd);
            CreateMonitorTab(hwnd);
            CreateLogsTab(hwnd);
            CreateSettingsTab(hwnd);
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

            if (LOWORD(wParam) == ID_QUIT_BUTTON) {
                s_isRunning = false;
                DestroyWindow(hwnd);
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

            if (LOWORD(wParam) == ID_TAB_CONTROL && HIWORD(wParam) == TCN_SELCHANGE) {
                int sel = TabCtrl_GetCurSel(s_hwndTabControl);
                ShowGroup(sel);
            }

            if (LOWORD(wParam) == ID_TRAY_SHOW) {
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
            }

            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                s_isRunning = false;
                DestroyWindow(hwnd);
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
                    s_currentSettings->startMinimized = (SendMessage(s_hwndStartMinimized, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->useD3D11 = (SendMessage(s_hwndUseD3D11, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    s_currentSettings->driverType = (NetMuxDriverType)SendMessage(s_hwndNetMuxDriverType, CB_GETCURSEL, 0, 0);

                    GetWindowText(s_hwndPeerColorR, buffer, 256);
                    s_currentSettings->peerColorR = (unsigned char)std::stoi(buffer);
                    GetWindowText(s_hwndPeerColorG, buffer, 256);
                    s_currentSettings->peerColorG = (unsigned char)std::stoi(buffer);
                    GetWindowText(s_hwndPeerColorB, buffer, 256);
                    s_currentSettings->peerColorB = (unsigned char)std::stoi(buffer);

                    if (s_hwndDisplayName) {
                        GetWindowText(s_hwndDisplayName, buffer, 256);
                        s_currentSettings->displayName = buffer;
                    }

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

    s_hwndMain = CreateWindow("NetMuxIntegratedGUI", "NetMux Monitor", WS_OVERLAPPEDWINDOW | (settings.startMinimized ? 0 : WS_VISIBLE), CW_USEDEFAULT, CW_USEDEFAULT, 370, 480, NULL, NULL, GetModuleHandle(NULL), NULL);
    s_isRunning = (s_hwndMain != NULL);

    if (s_hwndMain) {
        NOTIFYICONDATA nid = {0};
        nid.cbSize = sizeof(nid);
        nid.hWnd = s_hwndMain;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        strncpy(nid.szTip, "NetMux Monitor", sizeof(nid.szTip));
        Shell_NotifyIcon(NIM_ADD, &nid);
    }
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

    static bool firstScan = true;
    if (firstScan && s_currentNetwork) {
        s_currentNetwork->BroadcastDiscovery(5556);
        firstScan = false;
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

    if (s_isRunning) {
        static double lastLogUpdate = 0;
        double now = GetTickCount64();
        if (now - lastLogUpdate > 1000.0) {
            auto logs = Logger::GetRecentLogs(20);
            static size_t lastLogSize = 0;
            if (logs.size() != lastLogSize) {
                SendMessage(s_hwndDiagnosticsList, LB_RESETCONTENT, 0, 0);
                for (auto const& l : logs) SendMessage(s_hwndDiagnosticsList, LB_ADDSTRING, 0, (LPARAM)l.c_str());
                SendMessage(s_hwndDiagnosticsList, LB_SETCURSEL, (WPARAM)logs.size() - 1, 0);
                lastLogSize = logs.size();
            }
            lastLogUpdate = now;
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
                    const char* pName = (peer.displayName[0] != '\0') ? peer.displayName : peer.sessionName;
                    snprintf(info, sizeof(info), "%s | RTT:%dms | Drift:%.1fpx | [%s] | %s", pName, (int)peer.latency, peer.drift, authStatus, btnStr);
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
                    const char* pName = (peer.displayName[0] != '\0') ? peer.displayName : peer.sessionName;
                    snprintf(info, sizeof(info), "%s | RTT:%dms | Drift:%.1fpx | [%s] | %s", pName, (int)peer.latency, peer.drift, authStatus, btnStr);
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
        NOTIFYICONDATA nid = {0};
        nid.cbSize = sizeof(nid);
        nid.hWnd = s_hwndMain;
        nid.uID = 1;
        Shell_NotifyIcon(NIM_DELETE, &nid);
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
    ShowGroup(0);
#endif

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
