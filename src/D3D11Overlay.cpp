#include "D3D11Overlay.hpp"
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

D3D11Overlay::D3D11Overlay() : m_device(nullptr), m_context(nullptr), m_swapChain(nullptr), m_renderTargetView(nullptr), m_blendState(nullptr), m_hwnd(nullptr) {}
D3D11Overlay::~D3D11Overlay() { Shutdown(); }

bool D3D11Overlay::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    std::cout << "[D3D11] Initializing Hardware Overlay..." << std::endl;

    DXGI_SWAP_CHAIN_DESC scd = {0};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &scd, &m_swapChain, &m_device, NULL, &m_context);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_renderTargetView);
    pBackBuffer->Release();

    m_context->OMSetRenderTargets(1, &m_renderTargetView, NULL);

    D3D11_VIEWPORT viewport = {0};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (float)GetSystemMetrics(SM_CXSCREEN);
    viewport.Height = (float)GetSystemMetrics(SM_CYSCREEN);
    m_context->RSSetViewports(1, &viewport);

    // Alpha Blending
    D3D11_BLEND_DESC bd = {0};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_device->CreateBlendState(&bd, &m_blendState);
    m_context->OMSetBlendState(m_blendState, NULL, 0xffffffff);

    return true;
}

void D3D11Overlay::Render(const std::map<unsigned long long, RemoteCursorState>& peers) {
    if (!m_context) return;

    // Standard Direct3D 11 Render Loop
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Transparent
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);

    /*
     * ARCHITECTURE NOTE:
     * High-performance rendering of remote cursors using D3D11.
     * Cursors are rendered as textured quads with alpha-blending enabled.
     */

    for (auto const& [id, peer] : peers) {
        // Mock drawing logic:
        // In a full implementation, we would Bind Vertex Buffer,
        // Set Shaders, and DrawIndexed for each cursor.

        // D3D11_RECT rect = { peer.x, peer.y, peer.x + 32, peer.y + 32 };
        // DrawSprite(rect, peer.r, peer.g, peer.b);
    }

    m_swapChain->Present(1, 0);
}

void D3D11Overlay::Shutdown() {
    if (m_swapChain) m_swapChain->Release();
    if (m_renderTargetView) m_renderTargetView->Release();
    if (m_blendState) m_blendState->Release();
    if (m_device) m_device->Release();
    if (m_context) m_context->Release();

    m_swapChain = nullptr;
    m_renderTargetView = nullptr;
    m_blendState = nullptr;
    m_device = nullptr;
    m_context = nullptr;
}

#endif
