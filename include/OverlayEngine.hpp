#pragma once
#include <map>
#include <string>

enum class OverlayBackend {
    GDI,
    D3D11
};

struct RemoteCursorState {
    int x;
    int y;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned int groupId;
    bool isSelecting;
    bool isConflictBlocked;
    int selStartX;
    int selStartY;
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

    void SetScale(float scale) { m_scale = scale; }
    void SetBackend(OverlayBackend backend) { m_backend = backend; }
    void SetActivePeer(unsigned long long id);
    void LoadCursorTheme(const std::string& path);
    void SetSelectionColor(unsigned char r, unsigned char g, unsigned char b);

#ifdef _WIN32
    void* GetD3DOverlay() { return m_d3dOverlay; }
#endif

private:
    bool m_active;
    unsigned char m_colorR = 255;
    unsigned char m_colorG = 0;
    unsigned char m_colorB = 0;
    unsigned char m_selR = 0;
    unsigned char m_selG = 120;
    unsigned char m_selB = 215;
    float m_scale = 1.0f;
    OverlayBackend m_backend = OverlayBackend::GDI;
    unsigned long long m_activePeerId = 0;

#ifdef _WIN32
    void* GetD3D11Device();
    void* GetD3D11Context();
    void* m_hwnd;
    void* m_d3dOverlay; // Reference to D3D11Overlay class
    void* m_hdcMem;
    void* m_hBitmap;
    void* m_hOldBitmap;
    void* m_hPen;
    void* m_hBrush;
    int m_screenX;
    int m_screenY;
    int m_screenWidth;
    int m_screenHeight;

    void* m_hCursorBitmap;
    int m_cursorWidth;
    int m_cursorHeight;
#endif
};
