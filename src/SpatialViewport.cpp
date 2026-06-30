#include "SpatialViewport.hpp"
#include <iostream>
#include <cstring>
#include <cmath>

#ifdef _WIN32
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 texCoord;
};

struct ConstantBuffer {
    DirectX::XMMATRIX mWorldViewProj;
};

static const char* g_szVS =
"cbuffer cbPerObject : register(b0) {\n"
"   matrix WVP;\n"
"};\n"
"struct VS_Input {\n"
"   float3 pos : POSITION;\n"
"   float2 tex : TEXCOORD0;\n"
"};\n"
"struct VS_Output {\n"
"   float4 pos : SV_POSITION;\n"
"   float2 tex : TEXCOORD0;\n"
"};\n"
"VS_Output VS(VS_Input input) {\n"
"   VS_Output output;\n"
"   output.pos = mul(float4(input.pos, 1.0f), WVP);\n"
"   output.tex = input.tex;\n"
"   return output;\n"
"}\n";

static const char* g_szPS =
"Texture2D ObjTexture : register(t0);\n"
"SamplerState ObjSamplerState : register(s0);\n"
"struct VS_Output {\n"
"   float4 pos : SV_POSITION;\n"
"   float2 tex : TEXCOORD0;\n"
"};\n"
"float4 PS(VS_Output input) : SV_TARGET {\n"
"   return ObjTexture.Sample(ObjSamplerState, input.tex);\n"
"}\n";

#endif

SpatialViewport::SpatialViewport()
#ifdef _WIN32
    : m_currentCamX(0.0f), m_currentCamZ(-2.0f), m_transitionProgress(0.0f), m_localSRV(nullptr), m_remoteSRV(nullptr),
      m_vertexBuffer(nullptr), m_constantBuffer(nullptr), m_vertexShader(nullptr),
      m_pixelShader(nullptr), m_inputLayout(nullptr), m_samplerState(nullptr)
#endif
{
}

SpatialViewport::~SpatialViewport() {
#ifdef _WIN32
    if (m_samplerState) m_samplerState->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_pixelShader) m_pixelShader->Release();
    if (m_vertexShader) m_vertexShader->Release();
    if (m_constantBuffer) m_constantBuffer->Release();
    if (m_vertexBuffer) m_vertexBuffer->Release();
#endif
}

#ifdef _WIN32
bool SpatialViewport::Initialize(ID3D11Device* device) {
    std::cout << "[SpatialViewport] Initializing 3D Composition Engine (D3D11)..." << std::endl;

    // Set up View and Projection matrices for 3D space
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.01f, 100.0f);

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(g_szVS, strlen(g_szVS), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        return false;
    }
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();

    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(g_szPS, strlen(g_szPS), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        return false;
    }
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
    psBlob->Release();

    Vertex vertices[] = {
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
        { DirectX::XMFLOAT3( 1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
        { DirectX::XMFLOAT3( 1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) }
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;
    device->CreateBuffer(&bd, &InitData, &m_vertexBuffer);

    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&bd, nullptr, &m_constantBuffer);

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    sampDesc.MaxAnisotropy = 4;
    device->CreateSamplerState(&sampDesc, &m_samplerState);

    return true;
}

void SpatialViewport::Update(float deltaTime, bool isCrossingBorder) {
    // 1. Target positions for panning and zooming
    // When local (not crossing), focus at x=0, z=-2.5 (close)
    // When remote (crossing), focus at x=2.5, z=-2.5 (close)
    // To create a physical "swing" between monitors, the camera should zoom out during the transition.


    if (isCrossingBorder) {
        m_transitionProgress += deltaTime * 2.0f; // Speed of transition
        if (m_transitionProgress > 1.0f) m_transitionProgress = 1.0f;
    } else {
        m_transitionProgress -= deltaTime * 2.0f;
        if (m_transitionProgress < 0.0f) m_transitionProgress = 0.0f;
    }

    // 2. Smoothstep easing function for fluid transition (EaseInOut)
    float t = m_transitionProgress;
    float smoothT = t * t * (3.0f - 2.0f * t);

    // 3. Interpolate Camera X (Pan)
    float startX = 0.0f;
    float endX = 4.0f;
    m_currentCamX = startX + (endX - startX) * smoothT;

    // 4. Interpolate Camera Z (Zoom out in the middle of transition)
    // Parabolic arc for Z to simulate backing away while turning
    float baseZ = -2.5f;
    float pullBackZ = -5.0f;
    // Parabola: peaks at t=0.5
    float arc = 1.0f - 4.0f * (t - 0.5f) * (t - 0.5f);
    m_currentCamZ = baseZ + (pullBackZ - baseZ) * arc;

    // 5. Update view matrix based on interpolated camera position
    // The camera always looks straight ahead at its current X coordinate
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(m_currentCamX, 0.0f, m_currentCamZ, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(m_currentCamX, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
}

void SpatialViewport::Render(ID3D11DeviceContext* context) {
    if (!m_vertexBuffer || !m_vertexShader || !m_pixelShader) return;

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetInputLayout(m_inputLayout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(m_vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    context->PSSetShader(m_pixelShader, nullptr, 0);
    context->PSSetSamplers(0, 1, &m_samplerState);

    // Lambda to draw a plane
    auto drawPlane = [&](ID3D11ShaderResourceView* srv, float xOffset) {
        if (!srv) return;

        DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(xOffset, 0.0f, 0.0f);
        DirectX::XMMATRIX wvp = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projMatrix);

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            ConstantBuffer* cb = (ConstantBuffer*)mappedResource.pData;
            cb->mWorldViewProj = wvp;
            context->Unmap(m_constantBuffer, 0);
        }

        context->PSSetShaderResources(0, 1, &srv);
        context->Draw(4, 0);
    };

    // Draw Local Desktop at origin
    drawPlane(m_localSRV, 0.0f);

    // Draw Remote Desktop shifted to the right
    drawPlane(m_remoteSRV, 4.0f);
}
#endif
