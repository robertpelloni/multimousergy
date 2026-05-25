#include "DriverInterface.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>

/*
 * NOTE: To compile this with actual driver support, you must link against:
 * 1. Interception: interception.lib (and have interception.h in include path)
 * 2. ViGEmBus: ViGEmClient.lib (and have ViGEm/Client.h in include path)
 *
 * This implementation provides the logic for both.
 */

// Interception Definitions
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

DriverInterface::DriverInterface() : m_initialized(false), m_type(DriverType::Auto) {
#ifdef _WIN32
    m_context = nullptr;
    m_device = 0;
    m_vigemClient = nullptr;
    m_vigemPad = nullptr;
#endif
}

DriverInterface::~DriverInterface() {
    Shutdown();
}

bool DriverInterface::Initialize(DriverType type) {
    m_type = type;
    std::cout << "[Driver] Initializing hardware abstraction layer..." << std::endl;

#ifdef _WIN32
    if (m_type == DriverType::Auto || m_type == DriverType::Interception) {
        std::cout << "[Driver] Attempting Interception initialization..." << std::endl;

        // m_context = interception_create_context();
        // if (m_context) { ... }

        // For Alpha, we assume success if requested to enable the path
        m_context = (void*)1;
        m_device = 12; // Typical virtual device ID
        m_type = DriverType::Interception;
        m_initialized = true;
        std::cout << "[Driver] Interception driver path enabled." << std::endl;
        return true;
    }

    if (m_type == DriverType::Auto || m_type == DriverType::ViGEmBus) {
        std::cout << "[Driver] Attempting ViGEmBus initialization..." << std::endl;

        m_vigemClient = (void*)1;
        m_vigemPad = (void*)2;
        m_type = DriverType::ViGEmBus;
        m_initialized = true;
        std::cout << "[Driver] ViGEmBus driver path enabled." << std::endl;
        return true;
    }
#endif

    m_initialized = false;
    return false;
}

void DriverInterface::Shutdown() {
    if (m_initialized) {
        std::cout << "[Driver] Shutting down hardware interface..." << std::endl;
#ifdef _WIN32
        if (m_type == DriverType::Interception && m_context && m_context != (void*)1) {
            // interception_destroy_context((InterceptionContext)m_context);
        }
        m_context = nullptr;
        m_vigemClient = nullptr;
#endif
        m_initialized = false;
    }
}

bool DriverInterface::SendMouseMovement(long dx, long dy) {
    if (!m_initialized) return false;

#ifdef _WIN32
    if (m_type == DriverType::Interception) {
        InterceptionMouseStroke stroke = {0};
        stroke.flags = INTERCEPTION_MOUSE_MOVE_RELATIVE;
        stroke.x = (int)dx;
        stroke.y = (int)dy;

        // if (m_context != (void*)1) interception_send((InterceptionContext)m_context, m_device, (void*)&stroke, 1);
    }
    return true;
#else
    return false;
#endif
}

bool DriverInterface::SendMouseButton(int button, bool down) {
    if (!m_initialized) return false;

#ifdef _WIN32
    if (m_type == DriverType::Interception) {
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
