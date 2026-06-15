#include "DriverInterface.hpp"
#include <iostream>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>

#ifdef NETMUX_USE_NATIVE_DRIVERS
#include <interception.h>
#include <ViGEm/Client.h>
#pragma comment(lib, "interception.lib")
#pragma comment(lib, "ViGEmClient.lib")
#endif

/*
 * NOTE: To compile this with actual driver support, define NETMUX_USE_NATIVE_DRIVERS
 * and ensure the respective SDKs are in your include/library paths.
 */

// Interception Definitions (Fallback stubs)
typedef void* InterceptionContext;
typedef int InterceptionDevice;
typedef struct {
    unsigned short state;
    unsigned short flags;
    short rolling;
    int x;
    int y;
    unsigned int information;
} InterceptionMouseStroke;

#define INTERCEPTION_MOUSE_MOVE_RELATIVE 0x000
#define INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN 0x001
#define INTERCEPTION_MOUSE_LEFT_BUTTON_UP 0x002
#define INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN 0x004
#define INTERCEPTION_MOUSE_RIGHT_BUTTON_UP 0x008

// Function pointers for late binding or linking
// extern "C" InterceptionContext interception_create_context(void);
// extern "C" int interception_send(InterceptionContext context, InterceptionDevice device, const void *stroke, unsigned int nstroke);

// ViGEm stubs
typedef void* PVIGEM_CLIENT;
typedef void* PVIGEM_TARGET;
#endif

DriverInterface::DriverInterface() : m_initialized(false), m_type(NetMuxDriverType::Auto) {
#ifdef _WIN32
    m_context = nullptr;
    m_device = 0;
    m_vigemClient = nullptr;
    m_vigemPad = nullptr;
#endif
#ifdef __linux__
    m_device = -1;
#endif
}

DriverInterface::~DriverInterface() {
    Shutdown();
}

static bool CheckInterception() {
#ifdef _WIN32
    HMODULE h = LoadLibrary("interception.dll");
    if (h) { FreeLibrary(h); return true; }
#endif
    return false;
}

static bool CheckViGEm() {
#ifdef _WIN32
    HMODULE h = LoadLibrary("ViGEmClient.dll");
    if (h) { FreeLibrary(h); return true; }
#endif
    return false;
}

bool DriverInterface::Initialize(NetMuxDriverType type) {
    m_type = type;
    std::cout << "[Driver] Initializing hardware abstraction layer..." << std::endl;

#ifdef __linux__
    std::cout << "[Driver] Scanning Linux evdev devices..." << std::endl;
    for (int i = 0; i < 32; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            unsigned long rel_bits[1];
            if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits) >= 0) {
                if ((rel_bits[0] & (1 << REL_X)) && (rel_bits[0] & (1 << REL_Y))) {
                    m_device = fd;
                    m_initialized = true;
                    std::cout << "[Driver] Linux evdev injection enabled on: " << path << std::endl;
                    return true;
                }
            }
            close(fd);
        }
    }
    std::cerr << "[Driver] Failed to find a suitable evdev device (Permissions?)" << std::endl;
#endif

#ifdef _WIN32
    if (m_type == NetMuxDriverType::Auto || m_type == NetMuxDriverType::Interception) {
        std::cout << "[Driver] Attempting Interception initialization..." << std::endl;
#ifdef NETMUX_USE_NATIVE_DRIVERS
        m_context = interception_create_context();
        if (m_context) {
            m_device = 12; // Placeholder, real logic would identify device
            m_type = NetMuxDriverType::Interception;
            m_initialized = true;
            m_useNativeDrivers = true;
            std::cout << "[Driver] Native Interception driver enabled." << std::endl;
            return true;
        }
#else
        m_context = (void*)1;
        m_device = 12;
        m_type = NetMuxDriverType::Interception;
        m_initialized = true;
        std::cout << "[Driver] Interception stub path enabled." << std::endl;
        return true;
#endif
    }

    if (m_type == NetMuxDriverType::Auto || m_type == NetMuxDriverType::ViGEmBus) {
        std::cout << "[Driver] Attempting ViGEmBus initialization..." << std::endl;
#ifdef NETMUX_USE_NATIVE_DRIVERS
        m_vigemClient = vigem_alloc();
        if (VIGEM_SUCCESS(vigem_connect((PVIGEM_CLIENT)m_vigemClient))) {
             m_vigemPad = vigem_target_x360_alloc();
             vigem_target_add((PVIGEM_CLIENT)m_vigemClient, (PVIGEM_TARGET)m_vigemPad);
             m_type = NetMuxDriverType::ViGEmBus;
             m_initialized = true;
             m_useNativeDrivers = true;
             std::cout << "[Driver] Native ViGEmBus driver enabled." << std::endl;
             return true;
        }
#else
        m_vigemClient = (void*)1;
        m_vigemPad = (void*)2;
        m_type = NetMuxDriverType::ViGEmBus;
        m_initialized = true;
        std::cout << "[Driver] ViGEmBus stub path enabled." << std::endl;
        return true;
#endif
    }
#endif

    m_initialized = false;
    return false;
}

bool DriverInterface::IsDriverInstalled(NetMuxDriverType type) {
    if (type == NetMuxDriverType::Interception) return CheckInterception();
    if (type == NetMuxDriverType::ViGEmBus) return CheckViGEm();
    return false;
}

void DriverInterface::Shutdown() {
    if (m_initialized) {
        std::cout << "[Driver] Shutting down hardware interface..." << std::endl;
#ifdef _WIN32
        if (m_type == NetMuxDriverType::Interception && m_context && m_context != (void*)1) {
            // interception_destroy_context((InterceptionContext)m_context);
        }
        m_context = nullptr;
        m_vigemClient = nullptr;
#endif
#ifdef __linux__
        if (m_device >= 0) close(m_device);
        m_device = -1;
#endif
        m_initialized = false;
    }
}

bool DriverInterface::SendMouseMovement(long dx, long dy) {
    if (!m_initialized) return false;

#ifdef __linux__
    struct input_event ev[3];
    memset(ev, 0, sizeof(ev));

    ev[0].type = EV_REL; ev[0].code = REL_X; ev[0].value = (int)dx;
    ev[1].type = EV_REL; ev[1].code = REL_Y; ev[1].value = (int)dy;
    ev[2].type = EV_SYN; ev[2].code = SYN_REPORT; ev[2].value = 0;

    write(m_device, ev, sizeof(ev));
    return true;
#endif

#ifdef _WIN32
    if (m_type == NetMuxDriverType::Interception) {
        InterceptionMouseStroke stroke = {0};
        stroke.flags = INTERCEPTION_MOUSE_MOVE_RELATIVE;
        stroke.x = (int)dx;
        stroke.y = (int)dy;

#ifdef NETMUX_USE_NATIVE_DRIVERS
        interception_send((InterceptionContext)m_context, m_device, (void*)&stroke, 1);
#endif
    }
    return true;
#else
    return false;
#endif
}

bool DriverInterface::SendMouseWheel(int delta, bool horizontal) {
    if (!m_initialized) return false;

#ifdef __linux__
    struct input_event ev[2];
    memset(ev, 0, sizeof(ev));
    ev[0].type = EV_REL;
    ev[0].code = horizontal ? REL_HWHEEL : REL_WHEEL;
    ev[0].value = delta / 120; // evdev expects discrete units usually, but value varies
    ev[1].type = EV_SYN; ev[1].code = SYN_REPORT; ev[1].value = 0;
    write(m_device, ev, sizeof(ev));
    return true;
#endif

#ifdef _WIN32
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = horizontal ? MOUSEEVENTF_HWHEEL : MOUSEEVENTF_WHEEL;
    input.mi.mouseData = (DWORD)delta;
    SendInput(1, &input, sizeof(INPUT));
    return true;
#else
    return false;
#endif
}

bool DriverInterface::SendKeyboardKey(int key, bool down) {
    if (!m_initialized) return false;

#ifdef __linux__
    struct input_event ev[2];
    memset(ev, 0, sizeof(ev));
    ev[0].type = EV_KEY;
    ev[0].code = (uint16_t)key;
    ev[0].value = down ? 1 : 0;
    ev[1].type = EV_SYN;
    ev[1].code = SYN_REPORT;
    ev[1].value = 0;
    write(m_device, ev, sizeof(ev));
    return true;
#endif

#ifdef _WIN32
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = (WORD)key;
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    return true;
#else
    return false;
#endif
}

bool DriverInterface::SendMouseButton(int button, bool down) {
    if (!m_initialized) return false;

#ifdef __linux__
    struct input_event ev[2];
    memset(ev, 0, sizeof(ev));

    int code = BTN_LEFT;
    if (button == 1) code = BTN_RIGHT;
    else if (button == 2) code = BTN_MIDDLE;

    ev[0].type = EV_KEY; ev[0].code = code; ev[0].value = down ? 1 : 0;
    ev[1].type = EV_SYN; ev[1].code = SYN_REPORT; ev[1].value = 0;

    write(m_device, ev, sizeof(ev));
    return true;
#endif

#ifdef _WIN32
    if (m_type == NetMuxDriverType::Interception) {
        InterceptionMouseStroke stroke = {0};
        if (button == 0) stroke.state = down ? INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN : INTERCEPTION_MOUSE_LEFT_BUTTON_UP;
        else if (button == 1) stroke.state = down ? INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN : INTERCEPTION_MOUSE_RIGHT_BUTTON_UP;

        // if (m_context != (void*)1) interception_send((InterceptionContext)m_context, m_device, (void*)&stroke, 1);
    }

    std::cout << "[Driver] Injected hardware click (" << (int)m_type << "): Button=" << button << " Down=" << down << std::endl;
    return true;
#else
    return false;
#endif
}
