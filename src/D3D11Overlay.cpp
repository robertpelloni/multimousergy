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
    float u, v;
    float r, g, b, a;
};

D3D11Overlay::D3D11Overlay() : m_device(nullptr), m_context(nullptr), m_swapChain(nullptr),
                               m_renderTargetView(nullptr), m_blendState(nullptr), m_hwnd(nullptr),
                               m_vertexBuffer(nullptr), m_vertexShader(nullptr), m_pixelShader(nullptr),
                               m_inputLayout(nullptr), m_texture(nullptr), m_textureView(nullptr), m_sampler(nullptr) {}

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

    // Initialize Shaders
    const char* shaderCode =
        "Texture2D tex : register(t0);"
        "SamplerState samp : register(s0);"
        "struct VS_INPUT { float3 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };"
        "struct PS_INPUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };"
        "PS_INPUT VS(VS_INPUT input) { PS_INPUT output; output.pos = float4(input.pos, 1.0f); output.uv = input.uv; output.col = input.col; return output; }"
        "float4 PS(PS_INPUT input) : SV_Target { return tex.Sample(samp, input.uv) * input.col; }";

    ID3DBlob *vsBlob, *psBlob;
    D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vsBlob, NULL);
    D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &psBlob, NULL);

    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &m_vertexShader);
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    m_device->CreateInputLayout(ied, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

    vsBlob->Release();
    psBlob->Release();

    // PERFORMANCE OPTIMIZATION:
    // Create a larger dynamic vertex buffer once.
    // We can draw many cursors in one call or multiple calls using Map/Unmap.
    D3D11_BUFFER_DESC vbd = {0};
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(Vertex) * 4 * 64; // Support up to 64 cursors per draw call
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&vbd, NULL, &m_vertexBuffer);

    // Create a simple 8x8 white texture
    unsigned int pixels[64];
    for (int i = 0; i < 64; ++i) pixels[i] = 0xFFFFFFFF;

    D3D11_TEXTURE2D_DESC td = {0};
    td.Width = 8;
    td.Height = 8;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA tsd = {0};
    tsd.pSysMem = pixels;
    tsd.SysMemPitch = 8 * 4;

    m_device->CreateTexture2D(&td, &tsd, &m_texture);
    m_device->CreateShaderResourceView(m_texture, NULL, &m_textureView);

    D3D11_SAMPLER_DESC sad = {0};
    sad.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sad.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sad.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sad.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_device->CreateSamplerState(&sad, &m_sampler);

    return true;
}

void D3D11Overlay::Render(const std::map<unsigned long long, RemoteCursorState>& peers) {
    if (!m_context || peers.empty()) return;

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);

    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_vertexShader, NULL, 0);
    m_context->PSSetShader(m_pixelShader, NULL, 0);
    m_context->PSSetShaderResources(0, 1, &m_textureView);
    m_context->PSSetSamplers(0, 1, &m_sampler);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
    float screenH = (float)GetSystemMetrics(SM_CYSCREEN);

    // Batch all cursors into the dynamic buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    Vertex* v = (Vertex*)ms.pData;

    int count = 0;
    for (auto const& [id, peer] : peers) {
        if (count >= 64) break;

        float nx = (2.0f * peer.x / screenW) - 1.0f;
        float ny = 1.0f - (2.0f * peer.y / screenH);
        float sw = 16.0f / screenW;
        float sh = 16.0f / screenH;
        float r = peer.r / 255.0f;
        float g = peer.g / 255.0f;
        float b = peer.b / 255.0f;

        // Quad for one cursor
        v[count*4+0] = {nx, ny, 0.0f, 0.0f, 0.0f, r, g, b, 1.0f};
        v[count*4+1] = {nx + sw, ny, 0.0f, 1.0f, 0.0f, r, g, b, 1.0f};
        v[count*4+2] = {nx, ny - sh, 0.0f, 0.0f, 1.0f, r, g, b, 1.0f};
        v[count*4+3] = {nx + sw, ny - sh, 0.0f, 1.0f, 1.0f, r, g, b, 1.0f};
        count++;

        // Render selection box if active
        if (peer.isSelecting) {
            float snx = (2.0f * peer.selStartX / screenW) - 1.0f;
            float sny = 1.0f - (2.0f * peer.selStartY / screenH);

            // Draw a box from (snx, sny) to (nx, ny)
            v[count*4+0] = {snx, sny, 0.0f, 0.0f, 0.0f, r, g, b, 0.3f}; // Translucent
            v[count*4+1] = {nx,  sny, 0.0f, 1.0f, 0.0f, r, g, b, 0.3f};
            v[count*4+2] = {snx, ny,  0.0f, 0.0f, 1.0f, r, g, b, 0.3f};
            v[count*4+3] = {nx,  ny,  0.0f, 1.0f, 1.0f, r, g, b, 0.3f};
            count++;
        }
    }
    m_context->Unmap(m_vertexBuffer, 0);

    // Draw all quads (We use TriangleStrip with degenerate triangles or multiple draw calls)
    // For simplicity with TriangleStrip, we issue one draw call per cursor but with the pre-mapped buffer.
    // This is still much faster than re-creating the buffer.
    for(int i=0; i<count; ++i) {
        m_context->Draw(4, i*4);
    }

    m_swapChain->Present(1, 0);
}

void D3D11Overlay::DrawSprite(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    // Deprecated in favor of batched Render()
}

void D3D11Overlay::Shutdown() {
    if (m_texture) m_texture->Release();
    if (m_textureView) m_textureView->Release();
    if (m_sampler) m_sampler->Release();
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_vertexShader) m_vertexShader->Release();
    if (m_pixelShader) m_pixelShader->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_swapChain) m_swapChain->Release();
    if (m_renderTargetView) m_renderTargetView->Release();
    if (m_blendState) m_blendState->Release();
    if (m_device) m_device->Release();
    if (m_context) m_context->Release();

    m_texture = nullptr;
    m_textureView = nullptr;
    m_sampler = nullptr;
    m_vertexBuffer = nullptr;
    m_vertexShader = nullptr;
    m_pixelShader = nullptr;
    m_inputLayout = nullptr;
    m_swapChain = nullptr;
    m_renderTargetView = nullptr;
    m_blendState = nullptr;
    m_device = nullptr;
    m_context = nullptr;
}

#endif
