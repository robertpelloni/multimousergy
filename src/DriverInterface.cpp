#include "DriverInterface.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include "interception.h"

// ViGEm stubs
typedef void* PVIGEM_CLIENT;
typedef void* PVIGEM_TARGET;
#endif

DriverInterface::DriverInterface() : m_initialized(false), m_type(HardwareDriverType::Auto) {
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

bool DriverInterface::Initialize(HardwareDriverType type) {
    m_type = type;
    std::cout << "[Driver] Initializing hardware abstraction layer..." << std::endl;

#ifdef _WIN32
    if (m_type == HardwareDriverType::Auto || m_type == HardwareDriverType::Interception) {
        std::cout << "[Driver] Attempting Interception initialization..." << std::endl;
        m_context = interception_create_context();

        if (m_context) {
            // Interception uses device IDs 11-20 for mice.
            // We use the first mouse device (11) as our virtual target.
            m_device = INTERCEPTION_MOUSE(0);
            
            m_type = HardwareDriverType::Interception;
            m_initialized = true;
            std::cout << "[Driver] Interception driver path enabled. Using Device ID: " << m_device << std::endl;
            return true;
        } else {
            std::cerr << "[Driver] Error: Failed to create interception context." << std::endl;
        }
    }

    if (m_type == HardwareDriverType::Auto || m_type == HardwareDriverType::ViGEmBus) {
        std::cout << "[Driver] Attempting ViGEmBus initialization..." << std::endl;
        // In a real build: m_vigemClient = vigem_alloc();
        m_vigemClient = nullptr; // Mock as failing
        m_vigemPad = (void*)2;

        if (m_vigemClient) {
            m_type = HardwareDriverType::ViGEmBus;
            m_initialized = true;
            std::cout << "[Driver] ViGEmBus driver path enabled." << std::endl;
            return true;
        }
    }
#endif

    m_initialized = false;
    std::cout << "[Driver] All hardware drivers unavailable. Software fallback engaged." << std::endl;
    return false;
}

void DriverInterface::Shutdown() {
    if (m_initialized) {
        std::cout << "[Driver] Shutting down hardware interface..." << std::endl;
#ifdef _WIN32
        if (m_type == HardwareDriverType::Interception && m_context) {
            interception_destroy_context(m_context);
            m_context = nullptr;
        } else if (m_type == HardwareDriverType::ViGEmBus) {
            // vigem_disconnect((PVIGEM_CLIENT)m_vigemClient);
            // vigem_free((PVIGEM_CLIENT)m_vigemClient);
        }
        m_context = nullptr;
        m_vigemClient = nullptr;
#endif
        m_initialized = false;
    }
}

bool DriverInterface::SendMouseMovement(long dx, long dy) {
    if (!m_initialized) return false;

    m_lastX += dx;
    m_lastY += dy;

#ifdef _WIN32
    if (m_type == HardwareDriverType::Interception) {
        InterceptionMouseStroke stroke = {0};
        stroke.flags = INTERCEPTION_MOUSE_MOVE_RELATIVE;
        stroke.x = (int)dx;
        stroke.y = (int)dy;
        interception_send(m_context, m_device, (InterceptionStroke*)&stroke, 1);
    } else if (m_type == HardwareDriverType::ViGEmBus) {
        /*
         * ViGEm logic: Update virtual gamepad thumbstick to simulate mouse movement
         * or use virtual HID mouse if the extension is present.
         */
    }
    return true;
#else
    return false;
#endif
}

bool DriverInterface::SendMouseButton(int button, bool down) {
    if (!m_initialized) return false;

    if (down) m_buttonState |= (1 << button);
    else m_buttonState &= ~(1 << button);

#ifdef _WIN32
    if (m_type == HardwareDriverType::Interception) {
        InterceptionMouseStroke stroke = {0};
        stroke.state = 0;

        if (button == static_cast<int>(MouseButton::Left)) {
            stroke.state = down ? INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN : INTERCEPTION_MOUSE_LEFT_BUTTON_UP;
        } else if (button == static_cast<int>(MouseButton::Right)) {
            stroke.state = down ? INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN : INTERCEPTION_MOUSE_RIGHT_BUTTON_UP;
        } else if (button == static_cast<int>(MouseButton::Middle)) {
            stroke.state = down ? INTERCEPTION_MOUSE_MIDDLE_BUTTON_DOWN : INTERCEPTION_MOUSE_MIDDLE_BUTTON_UP;
        }

        if (stroke.state != 0) {
            interception_send(m_context, m_device, (InterceptionStroke*)&stroke, 1);
        }
    }
    std::cout << "[Driver] Injected hardware click (" << (int)m_type << "): Button=" << button << " Down=" << down << std::endl;
    return true;
#else
    return false;
#endif
}
