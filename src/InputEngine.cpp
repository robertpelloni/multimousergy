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

bool InputEngine::Initialize(const Config& config) {
    m_config = config;
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
        if (msg.message == WM_INPUT) {
            HRAWINPUT hRawInput = (HRAWINPUT)msg.lParam;
            UINT dwSize;
            GetRawInputData(hRawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[dwSize];
            if (lpb != NULL) {
                if (GetRawInputData(hRawInput, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                    RAWINPUT* raw = (RAWINPUT*)lpb;
                    if (raw->header.dwType == RIM_TYPEMOUSE) {
                        // Capture deltas
                        // Packet pkt = { PacketType::Movement, raw->data.mouse.lLastX, raw->data.mouse.lLastY, 0, false };
                        // m_pendingPackets.push(pkt);
                    }
                }
                delete[] lpb;
            }
        }
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
    if (m_config.isLeft) {
        return x <= m_config.boundaryX;
    } else {
        return x >= m_config.boundaryX;
    }
}

bool InputEngine::GetPendingPacket(Packet& pkt) {
    if (m_pendingPackets.empty()) return false;
    pkt = m_pendingPackets.front();
    m_pendingPackets.pop();
    return true;
}
