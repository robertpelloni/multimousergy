#include "InputEngine.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
static InputEngine* s_instance = nullptr;
#endif

InputEngine::InputEngine() : m_active(false) {
#ifdef _WIN32
    m_mouseHook = nullptr;
    m_hwnd = nullptr;
#endif
}

InputEngine::~InputEngine() {
    Shutdown();
}

#ifdef _WIN32
LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance) {
        MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
        if (s_instance->IsAtBoundary(mouseInfo->pt.x, mouseInfo->pt.y)) {
            // Suppress the message and trigger network crossover
            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif

bool InputEngine::Initialize(const Config& config) {
    m_config = config;
    std::cout << "[Input] Initializing Raw Input and Hooks..." << std::endl;

#ifdef _WIN32
    s_instance = this;

    // Create a message-only window to receive WM_INPUT
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = InputWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "NetMuxInputWindow";
    RegisterClassEx(&wc);

    m_hwnd = CreateWindowEx(0, "NetMuxInputWindow", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!m_hwnd) return false;

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02; // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = (HWND)m_hwnd;

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

            static std::vector<BYTE> lpb;
            if (lpb.size() < dwSize) lpb.resize(dwSize);

            if (GetRawInputData(hRawInput, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                RAWINPUT* raw = (RAWINPUT*)lpb.data();
                if (raw->header.dwType == RIM_TYPEMOUSE) {
                        if (raw->data.mouse.usFlags & MOUSE_MOVE_RELATIVE) {
                            Packet pkt;
                            pkt.type = PacketType::Movement;
                            pkt.x = raw->data.mouse.lLastX;
                            pkt.y = raw->data.mouse.lLastY;
                            pkt.button = 0;
                            pkt.down = false;
                            m_pendingPackets.push(pkt);
                        }

                        // Button events
                        USHORT flags = raw->data.mouse.usButtonFlags;
                        if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)   m_pendingPackets.push({PacketType::Click, 0, 0, 0, true});
                        if (flags & RI_MOUSE_LEFT_BUTTON_UP)     m_pendingPackets.push({PacketType::Click, 0, 0, 0, false});
                        if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN)  m_pendingPackets.push({PacketType::Click, 0, 0, 1, true});
                        if (flags & RI_MOUSE_RIGHT_BUTTON_UP)    m_pendingPackets.push({PacketType::Click, 0, 0, 1, false});
                        if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) m_pendingPackets.push({PacketType::Click, 0, 0, 2, true});
                        if (flags & RI_MOUSE_MIDDLE_BUTTON_UP)   m_pendingPackets.push({PacketType::Click, 0, 0, 2, false});
                    }
                }
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
        if (m_hwnd) {
            DestroyWindow((HWND)m_hwnd);
            m_hwnd = nullptr;
        }
        s_instance = nullptr;
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

void InputEngine::PerformWarpClickRestore(int targetX, int targetY, int button, bool down) {
#ifdef _WIN32
    POINT oldPos;
    GetCursorPos(&oldPos);

    // Snap to remote cursor position
    SetCursorPos(targetX, targetY);

    // Inject click
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    if (button == 0) input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    else if (button == 1) input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    else if (button == 2) input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;

    SendInput(1, &input, sizeof(INPUT));

    // Restore local cursor position
    SetCursorPos(oldPos.x, oldPos.y);
#endif
}
