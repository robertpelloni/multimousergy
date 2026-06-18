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

// Dynamic loading replaces static linking for driver SDKs

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

#ifdef _WIN32
typedef void* InterceptionContext;
typedef int InterceptionDevice;
typedef InterceptionContext (*pinterception_create_context)(void);
typedef void (*pinterception_destroy_context)(InterceptionContext);
typedef int (*pinterception_send)(InterceptionContext, InterceptionDevice, const void*, unsigned int);

typedef void* PVIGEM_CLIENT;
typedef void* PVIGEM_TARGET;
typedef int VIGEM_ERROR;
typedef PVIGEM_CLIENT (*pvigem_alloc)(void);
typedef void (*pvigem_free)(PVIGEM_CLIENT);
typedef VIGEM_ERROR (*pvigem_connect)(PVIGEM_CLIENT);
typedef PVIGEM_TARGET (*pvigem_target_x360_alloc)(void);
typedef VIGEM_ERROR (*pvigem_target_add)(PVIGEM_CLIENT, PVIGEM_TARGET);
#endif

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
        m_hInterception = (void*)LoadLibrary("interception.dll");
        if (m_hInterception) {
            auto create = (pinterception_create_context)GetProcAddress((HMODULE)m_hInterception, "interception_create_context");
            auto send = (pinterception_send)GetProcAddress((HMODULE)m_hInterception, "interception_send");
            auto destroy = (pinterception_destroy_context)GetProcAddress((HMODULE)m_hInterception, "interception_destroy_context");
            if (create && send && destroy) {
                m_context = create();
                if (m_context) {
                    m_device = 12;
                    m_type = NetMuxDriverType::Interception;
                    m_interception_send = (void*)send;
                    m_interception_destroy_context = (void*)destroy;
                    m_initialized = true;
                    std::cout << "[Driver] Native Interception driver dynamically loaded." << std::endl;
                    return true;
                }
            }
        }
        std::cout << "[Driver] Interception not found or failed to load. Using stub." << std::endl;
        m_context = (void*)1; m_device = 12; m_type = NetMuxDriverType::Interception; m_initialized = true;
        return true;
    }

    if (m_type == NetMuxDriverType::Auto || m_type == NetMuxDriverType::ViGEmBus) {
        m_hViGEm = (void*)LoadLibrary("ViGEmClient.dll");
        if (m_hViGEm) {
            auto alloc = (pvigem_alloc)GetProcAddress((HMODULE)m_hViGEm, "vigem_alloc");
            auto connect = (pvigem_connect)GetProcAddress((HMODULE)m_hViGEm, "vigem_connect");
            auto t_alloc = (pvigem_target_x360_alloc)GetProcAddress((HMODULE)m_hViGEm, "vigem_target_x360_alloc");
            auto t_add = (pvigem_target_add)GetProcAddress((HMODULE)m_hViGEm, "vigem_target_add");

            if (alloc && connect && t_alloc && t_add) {
                m_vigemClient = alloc();
                if (connect((PVIGEM_CLIENT)m_vigemClient) == 0) { // VIGEM_ERROR_NONE = 0
                    m_vigemPad = t_alloc();
                    t_add((PVIGEM_CLIENT)m_vigemClient, (PVIGEM_TARGET)m_vigemPad);
                    m_type = NetMuxDriverType::ViGEmBus;
                    m_initialized = true;
                    std::cout << "[Driver] Native ViGEmBus driver dynamically loaded." << std::endl;
                    return true;
                }
            }
        }
        std::cout << "[Driver] ViGEmBus not found or failed to load. Using stub." << std::endl;
        m_vigemClient = (void*)1; m_vigemPad = (void*)2; m_type = NetMuxDriverType::ViGEmBus; m_initialized = true;
        return true;
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
            if (m_interception_destroy_context) {
                ((pinterception_destroy_context)m_interception_destroy_context)((InterceptionContext)m_context);
            }
        }
        if (m_hInterception) FreeLibrary((HMODULE)m_hInterception);
        if (m_hViGEm) FreeLibrary((HMODULE)m_hViGEm);
        m_hInterception = nullptr;
        m_hViGEm = nullptr;
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

        if (m_context && m_context != (void*)1 && m_interception_send) {
            auto send = (pinterception_send)m_interception_send;
            send((InterceptionContext)m_context, m_device, (void*)&stroke, 1);
        }
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

        if (m_context && m_context != (void*)1 && m_interception_send) {
            auto send = (pinterception_send)m_interception_send;
            send((InterceptionContext)m_context, m_device, (void*)&stroke, 1);
        }
    }

    std::cout << "[Driver] Injected hardware click (" << (int)m_type << "): Button=" << button << " Down=" << down << std::endl;
    return true;
#else
    return false;
#endif
}
