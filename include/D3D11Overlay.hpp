#pragma once
#include <map>
#include "OverlayEngine.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#endif

class D3D11Overlay {
public:
    D3D11Overlay();
    ~D3D11Overlay();

#ifdef _WIN32
    bool Initialize(HWND hwnd);
#else
    bool Initialize(void* hwnd);
#endif
    void Render(const std::map<unsigned long long, RemoteCursorState>& peers);
    void SetActivePeer(unsigned long long id) { m_activePeerId = id; }
    void SetSelectionColor(unsigned char r, unsigned char g, unsigned char b) {
        m_selR = r; m_selG = g; m_selB = b;
    }
    void UpdateCursorTexture(void* hBitmap);
    void Shutdown();

#ifdef _WIN32
    ID3D11Device* GetDevice() { return m_device; }
    ID3D11DeviceContext* GetContext() { return m_context; }
#endif

private:
    unsigned long long m_activePeerId = 0;
    unsigned char m_selR = 0, m_selG = 120, m_selB = 215;

#ifdef _WIN32
    void DrawSprite(int x, int y, unsigned char r, unsigned char g, unsigned char b);

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGISwapChain* m_swapChain;
    ID3D11RenderTargetView* m_renderTargetView;
    ID3D11BlendState* m_blendState;
    HWND m_hwnd;

    ID3D11Buffer* m_vertexBuffer;
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_inputLayout;
    ID3D11Texture2D* m_texture;
    ID3D11ShaderResourceView* m_textureView;
    ID3D11SamplerState* m_sampler;
    int m_screenX;
    int m_screenY;
    int m_screenWidth;
    int m_screenHeight;
#endif
};
