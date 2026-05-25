#pragma once
#include <string>

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

class DriverInterface {
public:
    DriverInterface();
    ~DriverInterface();

    bool Initialize();
    void Shutdown();

    bool SendMouseMovement(long dx, long dy);
    bool SendMouseButton(int button, bool down);

private:
    bool m_initialized;
    long m_lastX = 0;
    long m_lastY = 0;
    int m_buttonState = 0; // bitmask

#ifdef _WIN32
    void* m_context;
    int m_virtual_mouse_id;
#endif
};
