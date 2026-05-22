#pragma once
#include <string>

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
};
