#include "InputEngine.hpp"
#include <iostream>
#include <chrono>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
static InputEngine* s_instance = nullptr;
#endif

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#endif

#include <vector>

#ifdef _WIN32
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance) {
        MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;

        static auto startTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        s_instance->m_lastLocalActivity = std::chrono::duration<double, std::milli>(now - startTime).count();

        if (mouseInfo->flags & LLMHF_INJECTED) {
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }

        bool atBoundary = s_instance->IsAtBoundary(mouseInfo->pt.x, mouseInfo->pt.y);

        if (atBoundary && !s_instance->m_isCaptured && s_instance->m_peerConnected) {
            s_instance->m_isCaptured = true;
            s_instance->m_accumulatedX = 0;
            
            // Sync virtual position with hook position on capture
            int sl = GetSystemMetrics(SM_XVIRTUALSCREEN);
            int st = GetSystemMetrics(SM_YVIRTUALSCREEN);
            int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            s_instance->m_virtualX = ((mouseInfo->pt.x - sl) * 65535) / (sw > 1 ? sw - 1 : 1);
            s_instance->m_virtualY = ((mouseInfo->pt.y - st) * 65535) / (sh > 1 ? sh - 1 : 1);

            std::cout << "[Input] Boundary hit. Capturing. Pos: (" << s_instance->m_virtualX << "," << s_instance->m_virtualY << ")" << std::endl;
            return 1;
        }

        if (s_instance->m_isCaptured) {
            bool movingBack = false;
            // ~2% threshold
            if (s_instance->m_config.isLeft) {
                if (s_instance->m_accumulatedX < -1500) movingBack = true;
            } else {
                if (s_instance->m_accumulatedX > 1500) movingBack = true;
            }

            if (movingBack) {
                std::cout << "[Input] Release threshold met. Returning to local control." << std::endl;
                s_instance->m_isCaptured = false;
                s_instance->m_accumulatedX = 0;
                
                if (s_instance->m_config.isLeft) SetCursorPos(mouseInfo->pt.x - 20, mouseInfo->pt.y);
                else SetCursorPos(mouseInfo->pt.x + 20, mouseInfo->pt.y);
                
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }

            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance && s_instance->m_isCaptured) {
        KBDLLHOOKSTRUCT* kbInfo = (KBDLLHOOKSTRUCT*)lParam;
        if (!(kbInfo->flags & LLKHF_INJECTED)) {
            // Forward keys to network
            // TODO: Packet pkt = { ... }; s_instance->m_pendingPackets.push(pkt);
            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

InputEngine::InputEngine() : m_active(false), m_isCaptured(false), m_peerConnected(false), m_lastLocalActivity(0), m_accumulatedX(0), m_virtualX(0), m_virtualY(0), m_isSelecting(false) {
#ifdef _WIN32
    m_mouseHook = nullptr;
    m_keyboardHook = nullptr;
    m_hwnd = nullptr;
#endif
}

InputEngine::~InputEngine() {
    Shutdown();
}

#ifdef __linux__
bool InputEngine::Initialize(const Config& config, void* xDisplay) {
    m_xDisplay = xDisplay;
#else
bool InputEngine::Initialize(const Config& config) {
#endif
    m_config = config;
    std::cout << "[Input] Initializing Input Capture..." << std::endl;

#ifdef _WIN32
    POINT pt;
    GetCursorPos(&pt);
    int sl = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int st = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    m_virtualX = ((pt.x - sl) * 65535) / (sw > 1 ? sw - 1 : 1);
    m_virtualY = ((pt.y - st) * 65535) / (sh > 1 ? sh - 1 : 1);
#endif

#ifdef __linux__
    for (int i = 0; i < 32; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            unsigned long rel_bits[1];
            if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits) >= 0) {
                if ((rel_bits[0] & (1 << REL_X)) && (rel_bits[0] & (1 << REL_Y))) {
                    m_fds.push_back(fd);
                } else close(fd);
            } else close(fd);
        }
    }
#endif

#ifdef _WIN32
    s_instance = this;
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
    rid.usUsage = 0x02; 
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = (HWND)m_hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) return false;

    m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);
#endif

    m_active = true;
    return true;
}

void InputEngine::Update() {
    if (!m_active) return;

#ifdef __linux__
    for (int fd : m_fds) {
        struct input_event ev;
        while (read(fd, &ev, sizeof(ev)) > 0) {
            if (ev.type == EV_REL) {
                int dx = (ev.code == REL_X) ? ev.value : 0;
                int dy = (ev.code == REL_Y) ? ev.value : 0;
                m_virtualX += (dx * 65535) / 1919;
                m_virtualY += (dy * 65535) / 1079;
                if (m_virtualX < 0) m_virtualX = 0; if (m_virtualX > 65535) m_virtualX = 65535;
                if (m_virtualY < 0) m_virtualY = 0; if (m_virtualY > 65535) m_virtualY = 65535;
                if (m_isCaptured) m_accumulatedX += dx;
                Packet pkt = { 0, 0, 0, 0.0, NetMuxPacketType::AbsoluteMovement, m_virtualX, m_virtualY, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
                m_pendingPackets.push(pkt);
            }
        }
    }
#endif

#ifdef _WIN32
    // BROADCAST VISIBILITY:
    // Even if not captured, we broadcast our local absolute position.
    // This allows other peers to see where we are on our screen (mapped to their screen).
    // This fulfills the "Mux" (Multiplexing) vision of shared visibility.
    POINT pt;
    GetCursorPos(&pt);
    int sl = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int st = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    int newVX = ((pt.x - sl) * 65535) / (sw > 1 ? sw - 1 : 1);
    int newVY = ((pt.y - st) * 65535) / (sh > 1 ? sh - 1 : 1);

    if (newVX != m_virtualX || newVY != m_virtualY) {
        m_virtualX = newVX;
        m_virtualY = newVY;
        Packet pkt = { 0, 0, 0, 0.0, NetMuxPacketType::AbsoluteMovement, m_virtualX, m_virtualY, 0, false, false, 0, 0, 0, false, 0, 0, "", 0 };
        m_pendingPackets.push(pkt);
    }

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
                    bool isRelative = !(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE);
                    if (isRelative) {
                        int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                        int sh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                        if (sw < 1) sw = 1920;
                        if (sh < 1) sh = 1080;

                        m_accumulatedX += (raw->data.mouse.lLastX * 65535) / sw;
                        m_virtualX += (raw->data.mouse.lLastX * 65535) / sw;
                        m_virtualY += (raw->data.mouse.lLastY * 65535) / sh;
                        if (m_virtualX < 0) m_virtualX = 0; if (m_virtualX > 65535) m_virtualX = 65535;
                        if (m_virtualY < 0) m_virtualY = 0; if (m_virtualY > 65535) m_virtualY = 65535;

                        Packet pkt = {0, 0, 0, 0.0, NetMuxPacketType::AbsoluteMovement, m_virtualX, m_virtualY, 0, false, false, 0, 0, 0, false, 0, 0, "", 0};
                        m_pendingPackets.push(pkt);
                    }

                    USHORT flags = raw->data.mouse.usButtonFlags;
                    if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                        m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Click, 0, 0, 0, true, false, 0, 0, 0, false, 0, 0, "", 0});
                        m_isSelecting = true; m_selStartX = m_virtualX; m_selStartY = m_virtualY;
                    }
                    if (flags & RI_MOUSE_LEFT_BUTTON_UP) {
                        m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Click, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0});
                        if (m_isSelecting) {
                            m_isSelecting = false;
                            m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::SelectionUpdate, 0, 0, 0, false, false, 0, 0, 0, false, 0, 0, "", 0});
                        }
                    }
                    if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN)  m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Click, 0, 0, 1, true, false, 0, 0, 0, false, 0, 0, "", 0});
                    if (flags & RI_MOUSE_RIGHT_BUTTON_UP)    m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Click, 0, 0, 1, false, false, 0, 0, 0, false, 0, 0, "", 0});
                    if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Click, 0, 0, 2, true, false, 0, 0, 0, false, 0, 0, "", 0});
                    if (flags & RI_MOUSE_MIDDLE_BUTTON_UP)   m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Click, 0, 0, 2, false, false, 0, 0, 0, false, 0, 0, "", 0});

                    if (flags & RI_MOUSE_WHEEL) {
                        short delta = (short)raw->data.mouse.usButtonData;
                        m_pendingPackets.push(Packet{0, 0, 0, 0.0, NetMuxPacketType::Wheel, 0, 0, 0, false, false, 0, 0, (int)delta, false, 0, 0, "", 0});
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
        std::cout << "[Input] Shutting down input capture..." << std::endl;
#ifdef __linux__
        for (int fd : m_fds) close(fd);
        m_fds.clear();
#endif
#ifdef _WIN32
        if (m_mouseHook) UnhookWindowsHookEx((HHOOK)m_mouseHook);
        if (m_keyboardHook) UnhookWindowsHookEx((HHOOK)m_keyboardHook);
        if (m_hwnd) DestroyWindow((HWND)m_hwnd);
        s_instance = nullptr;
#endif
        m_active = false;
    }
}

bool InputEngine::IsAtBoundary(int x, int y) {
#ifdef _WIN32
    int sl = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int sw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int sr = sl + sw - 1;
    if (m_config.isLeft) return x >= sr;
    else return x <= sl;
#else
    return m_config.isLeft ? (x >= m_config.boundaryX) : (x <= m_config.boundaryX);
#endif
}

void InputEngine::SetPeerConnected(bool connected) {
    m_peerConnected = connected;
}

bool InputEngine::GetPendingPacket(Packet& pkt) {
    if (m_pendingPackets.empty()) return false;
    pkt = m_pendingPackets.front();
    m_pendingPackets.pop();
    return true;
}

bool InputEngine::IsCaptured() const {
    return m_isCaptured;
}

double InputEngine::GetLastLocalActivity() const {
    return m_lastLocalActivity;
}

void InputEngine::PerformWarpClickRestore(int x, int y, int button, bool down) {
#ifdef _WIN32
    POINT oldPos;
    GetCursorPos(&oldPos);
    SetCursorPos(x, y);
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    if (button == 0) input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    else if (button == 1) input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    else if (button == 2) input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    SendInput(1, &input, sizeof(INPUT));
    SetCursorPos(oldPos.x, oldPos.y);
#endif
}
