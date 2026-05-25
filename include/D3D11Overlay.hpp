#pragma once
#include <map>
#include "OverlayEngine.hpp"

#ifdef _WIN32
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
    void Shutdown();

private:
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
#endif
};
