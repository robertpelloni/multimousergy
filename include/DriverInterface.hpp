#pragma once
#include <string>

enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

enum class HALDriverType {
    Interception,
    ViGEmBus,
    Auto
};

class DriverInterface {
public:
    DriverInterface();
    ~DriverInterface();

    bool Initialize(HALDriverType type = HALDriverType::Auto);
    void Shutdown();

    bool SendMouseMovement(long dx, long dy);
    bool SendMouseButton(int button, bool down);

private:
    bool m_initialized;
    HALDriverType m_type;
    long m_lastX = 0;
    long m_lastY = 0;
    int m_buttonState = 0; // bitmask

#ifdef _WIN32
    void* m_context; // Interception context
    int m_device;    // Virtual mouse device ID
    void* m_vigemClient; // ViGEm client handle
    void* m_vigemPad;    // ViGEm target handle
#endif
};
