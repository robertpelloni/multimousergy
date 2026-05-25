#include "DriverInterface.hpp"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include "interception.h"
#endif

DriverInterface::DriverInterface() : m_initialized(false) {
#ifdef _WIN32
    m_context = nullptr;
    m_virtual_mouse_id = 0;
#endif
}

DriverInterface::~DriverInterface() {
    Shutdown();
}

bool DriverInterface::Initialize() {
    std::cout << "[Driver] Initializing virtual HID device (Interception)..." << std::endl;

#ifdef _WIN32
    m_context = interception_create_context();
    if (!m_context) {
        std::cerr << "[Driver] Error: Failed to create interception context. Is the Interception driver installed?" << std::endl;
        m_initialized = false;
        return false;
    }

    // Interception uses device IDs 11-20 for mice.
    // We use the first mouse device (11) as our virtual target.
    m_virtual_mouse_id = INTERCEPTION_MOUSE(0);
    
    m_initialized = true;
    std::cout << "[Driver] Interception context created successfully. Using Device ID: " << m_virtual_mouse_id << std::endl;
    return true;
#else
    m_initialized = false;
    return false;
#endif
}

void DriverInterface::Shutdown() {
    if (m_initialized) {
        std::cout << "[Driver] Shutting down virtual HID device..." << std::endl;
#ifdef _WIN32
        if (m_context) {
            interception_destroy_context(m_context);
            m_context = nullptr;
        }
#endif
        m_initialized = false;
    }
}

bool DriverInterface::SendMouseMovement(long dx, long dy) {
#ifdef _WIN32
    if (!m_initialized || m_context == nullptr) return false;
#else
    if (!m_initialized) return false;
#endif

    m_lastX += dx;
    m_lastY += dy;

#ifdef _WIN32
    InterceptionMouseStroke stroke = {0};
    stroke.flags = INTERCEPTION_MOUSE_MOVE_RELATIVE;
    stroke.x = dx;
    stroke.y = dy;
    interception_send(m_context, m_virtual_mouse_id, (InterceptionStroke*)&stroke, 1);
#endif

    return true;
}

bool DriverInterface::SendMouseButton(int button, bool down) {
#ifdef _WIN32
    if (!m_initialized || m_context == nullptr) return false;
#else
    if (!m_initialized) return false;
#endif

    if (down) m_buttonState |= (1 << button);
    else m_buttonState &= ~(1 << button);

#ifdef _WIN32
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
        interception_send(m_context, m_virtual_mouse_id, (InterceptionStroke*)&stroke, 1);
    }
#endif

    return true;
}
