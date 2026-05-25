#include "D3D11Overlay.hpp"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

D3D11Overlay::D3D11Overlay() : m_device(nullptr), m_context(nullptr), m_swapChain(nullptr),
                               m_renderTargetView(nullptr), m_blendState(nullptr), m_hwnd(nullptr),
                               m_vertexBuffer(nullptr), m_vertexShader(nullptr), m_pixelShader(nullptr),
                               m_inputLayout(nullptr) {}

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

    // Initialize Shaders and Buffers for Sprite Rendering
    const char* shaderCode =
        "struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };"
        "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };"
        "PS_INPUT VS(VS_INPUT input) { PS_INPUT output; output.pos = float4(input.pos, 1.0f); output.col = input.col; return output; }"
        "float4 PS(PS_INPUT input) : SV_Target { return input.col; }";

    ID3DBlob *vsBlob, *psBlob;
    D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vsBlob, NULL);
    D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &psBlob, NULL);

    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &m_vertexShader);
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    m_device->CreateInputLayout(ied, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

    vsBlob->Release();
    psBlob->Release();

    return true;
}

void D3D11Overlay::Render(const std::map<unsigned long long, RemoteCursorState>& peers) {
    if (!m_context) return;

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);

    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_vertexShader, NULL, 0);
    m_context->PSSetShader(m_pixelShader, NULL, 0);

    for (auto const& [id, peer] : peers) {
        DrawSprite(peer.x, peer.y, peer.r, peer.g, peer.b);
    }

    m_swapChain->Present(1, 0);
}

void D3D11Overlay::DrawSprite(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (!m_context) return;

    // Convert screen coordinates to NDC (-1 to 1)
    float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
    float screenH = (float)GetSystemMetrics(SM_CYSCREEN);

    float nx = (2.0f * x / screenW) - 1.0f;
    float ny = 1.0f - (2.0f * y / screenH);
    float sw = 10.0f / screenW; // 10px wide
    float sh = 10.0f / screenH; // 10px high

    Vertex vertices[] = {
        {nx, ny, 0.0f, r/255.0f, g/255.0f, b/255.0f, 1.0f},
        {nx + sw, ny, 0.0f, r/255.0f, g/255.0f, b/255.0f, 1.0f},
        {nx, ny - sh, 0.0f, r/255.0f, g/255.0f, b/255.0f, 1.0f},
        {nx + sw, ny - sh, 0.0f, r/255.0f, g/255.0f, b/255.0f, 1.0f}
    };

    D3D11_BUFFER_DESC bd = {0};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sd = {0};
    sd.pSysMem = vertices;

    ID3D11Buffer* pVBuffer;
    m_device->CreateBuffer(&bd, &sd, &pVBuffer);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    m_context->Draw(4, 0);
    pVBuffer->Release();
}

void D3D11Overlay::Shutdown() {
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_vertexShader) m_vertexShader->Release();
    if (m_pixelShader) m_pixelShader->Release();
    if (m_inputLayout) m_inputLayout->Release();
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
