#pragma once
#include <map>

struct RemoteCursorState {
    int x;
    int y;
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

class OverlayEngine {
public:
    OverlayEngine();
    ~OverlayEngine();

    bool Initialize();
    void Render(int cursorX, int cursorY);
    void RenderPeers(const std::map<unsigned long long, RemoteCursorState>& peers);
    void Shutdown();

    void SetColor(unsigned char r, unsigned char g, unsigned char b) {
        m_colorR = r; m_colorG = g; m_colorB = b;
    }

private:
    bool m_active;
    unsigned char m_colorR = 255;
    unsigned char m_colorG = 0;
    unsigned char m_colorB = 0;

#ifdef _WIN32
    void* m_hwnd;
    void* m_hdcMem;
    void* m_hBitmap;
    void* m_hOldBitmap;
    void* m_hPen;
    void* m_hBrush;
    int m_screenWidth;
    int m_screenHeight;
#endif
};
