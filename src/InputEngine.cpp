#include "InputEngine.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

InputEngine::InputEngine() : m_active(false) {
#ifdef _WIN32
    m_mouseHook = nullptr;
#endif
}

InputEngine::~InputEngine() {
    Shutdown();
}

#ifdef _WIN32
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        // MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
        // logic for boundary detection and suppression
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif

bool InputEngine::Initialize() {
    std::cout << "[Input] Initializing Raw Input and Hooks..." << std::endl;

#ifdef _WIN32
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02; // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = GetConsoleWindow(); // Placeholder

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        return false;
    }

    m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
    if (!m_mouseHook) return false;
#endif

    m_active = true;
    return true;
}

void InputEngine::Update() {
    if (!m_active) return;

#ifdef _WIN32
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

void InputEngine::Shutdown() {
    if (m_active) {
        std::cout << "[Input] Shutting down interception..." << std::endl;
#ifdef _WIN32
        if (m_mouseHook) {
            UnhookWindowsHookEx((HHOOK)m_mouseHook);
            m_mouseHook = nullptr;
        }
#endif
        m_active = false;
    }
}

bool InputEngine::IsAtBoundary(int x, int y) {
    // Basic logic for boundary detection
    return false;
}
