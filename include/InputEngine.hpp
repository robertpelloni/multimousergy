#pragma once
#include <vector>

class InputEngine {
public:
    InputEngine();
    ~InputEngine();

    bool Initialize();
    void Update();
    void Shutdown();

    bool IsAtBoundary(int x, int y);

private:
    bool m_active;
#ifdef _WIN32
    void* m_mouseHook;
#endif
};
