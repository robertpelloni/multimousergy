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

enum ControlIDs {
    ID_SAVE_BUTTON = 1,
    ID_PEER_LIST = 2,
    ID_CURSOR_SCALE = 3
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

bool ConfigGUI::ShowDialog(AppSettings& settings) {
    std::cout << "[GUI] Launching configuration dialog..." << std::endl;

#ifdef _WIN32
    s_currentSettings = &settings;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "NetMuxSettings";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("NetMuxSettings", "NetMux Settings", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 300, 350, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;

    NetworkManager discoveryNetwork;

    MSG msg;
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Poll for discovery in the background
            DiscoveryPacket dp;
            if (discoveryNetwork.PollDiscovery(dp)) {
                // If peer not in list, add it
                if (SendMessage(s_hwndPeerList, CB_FINDSTRINGEXACT, -1, (LPARAM)dp.hostname) == CB_ERR) {
                    SendMessage(s_hwndPeerList, CB_ADDSTRING, 0, (LPARAM)dp.hostname);
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
