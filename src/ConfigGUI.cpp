#include "ConfigGUI.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static AppSettings* s_currentSettings = nullptr;
static HWND s_hwndServer = nullptr;
static HWND s_hwndIp = nullptr;
static HWND s_hwndPort = nullptr;
static HWND s_hwndBoundary = nullptr;

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

            CreateWindow("BUTTON", "Save & Start", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 80, 140, 120, 30, hwnd, (HMENU)1, NULL, NULL);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                char buffer[256];
                try {
                    GetWindowText(s_hwndPort, buffer, 256);
                    int port = std::stoi(buffer);

                    GetWindowText(s_hwndBoundary, buffer, 256);
                    int boundaryX = std::stoi(buffer);

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

    HWND hwnd = CreateWindow("NetMuxSettings", "NetMux Settings", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 300, 220, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (msg.wParam == 1);
#else
    return true;
#endif
}
