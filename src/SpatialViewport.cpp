#include "SpatialViewport.hpp"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <d3dcompiler.h>
#endif

SpatialViewport::SpatialViewport()
#ifdef _WIN32
    : m_currentCamX(0.0f), m_currentCamZ(0.0f), m_localSRV(nullptr), m_remoteSRV(nullptr), m_cursorSRV(nullptr),
      m_localWebcamSRV(nullptr), m_remoteWebcamSRV(nullptr),
      m_vertexBuffer(nullptr), m_constantBuffer(nullptr), m_vertexShader(nullptr),
      m_pixelShader(nullptr), m_inputLayout(nullptr), m_sampler(nullptr)
#endif
{
}

SpatialViewport::~SpatialViewport() {
#ifdef _WIN32
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_constantBuffer) m_constantBuffer->Release();
    if (m_vertexShader) m_vertexShader->Release();
    if (m_pixelShader) m_pixelShader->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_sampler) m_sampler->Release();
    if (m_localSRV) m_localSRV->Release();
    if (m_remoteSRV) m_remoteSRV->Release();
    if (m_cursorSRV) m_cursorSRV->Release();
    if (m_localWebcamSRV) m_localWebcamSRV->Release();
    if (m_remoteWebcamSRV) m_remoteWebcamSRV->Release();
#endif
}

#ifdef _WIN32
struct SpatialVertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
};

bool SpatialViewport::Initialize(ID3D11Device* device, float aspectRatio) {
    std::cout << "[SpatialViewport] Initializing 3D Composition Engine (D3D11)..." << std::endl;

    // 1. Create Vertex Buffer for a 2D plane in 3D space
    SpatialVertex vertices[] = {
        { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f} },
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
    };

    D3D11_BUFFER_DESC vbd = {0};
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(vertices);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vsd = {0};
    vsd.pSysMem = vertices;
    device->CreateBuffer(&vbd, &vsd, &m_vertexBuffer);

    // 2. Create Constant Buffer for WVP matrix
    D3D11_BUFFER_DESC cbd = {0};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbd, NULL, &m_constantBuffer);

    // 3. Compile Shaders
    const char* shaderCode =
        "cbuffer cb : register(b0) { matrix wvp; };"
        "struct VS_IN { float3 pos : POSITION; float2 uv : TEXCOORD; };"
        "struct PS_IN { float4 pos : SV_POSITION; float2 uv : TEXCOORD; };"
        "PS_IN VS(VS_IN input) { PS_IN output; output.pos = mul(float4(input.pos, 1.0f), wvp); output.uv = input.uv; return output; }"
        "Texture2D tex : register(t0); SamplerState samp : register(s0);"
        "float4 PS(PS_IN input) : SV_Target { return tex.Sample(samp, input.uv); }";

    ID3DBlob *vsBlob, *psBlob;
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vsBlob, NULL))) return false;
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &psBlob, NULL))) return false;

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &m_vertexShader);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    device->CreateInputLayout(ied, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release(); psBlob->Release();

    // 4. Sampler State
    D3D11_SAMPLER_DESC sad = {0};
    sad.Filter = D3D11_FILTER_ANISOTROPIC;
    sad.AddressU = sad.AddressV = sad.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sad.MaxAnisotropy = 8;
    device->CreateSamplerState(&sad, &m_sampler);

    // Set up View and Projection matrices for 3D space
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, aspectRatio, 0.01f, 100.0f);

    return true;
}

void SpatialViewport::Update(float deltaTime, bool isCrossingBorder, float aspectRatio) {
    if (aspectRatio > 0) {
        m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, aspectRatio, 0.01f, 100.0f);
    }

    // Boundary-triggered Spatial Animation (The "Zoom")
    float targetX = isCrossingBorder ? 4.0f : 0.0f; // Shift to remote plane
    float targetZ = isCrossingBorder ? -3.0f : -5.0f; // Zoom out to see context

    float lerpSpeed = 5.0f;
    m_currentCamX += (targetX - m_currentCamX) * deltaTime * lerpSpeed;
    m_currentCamZ += (targetZ - m_currentCamZ) * deltaTime * lerpSpeed;

    // Update view matrix based on interpolated camera position
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(m_currentCamX, 0.0f, m_currentCamZ, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(m_currentCamX, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
}

void SpatialViewport::Render(ID3D11DeviceContext* context, const std::map<unsigned long long, RemoteCursorState>& peers) {
    if (!context || !m_localSRV) return;

    context->IASetInputLayout(m_inputLayout);
    context->VSSetShader(m_vertexShader, NULL, 0);
    context->PSSetShader(m_pixelShader, NULL, 0);
    context->PSSetSamplers(0, 1, &m_sampler);

    UINT stride = sizeof(SpatialVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Render Local Plane
    D3D11_MAPPED_SUBRESOURCE ms;
    context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    ConstantBuffer* cb = (ConstantBuffer*)ms.pData;

    // Scale local plane to maintain aspect ratio 1.0f in normalized space (fill viewport)
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
    cb->worldViewProj = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projMatrix);
    context->Unmap(m_constantBuffer, 0);

    context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    context->PSSetShaderResources(0, 1, &m_localSRV);
    context->Draw(4, 0);

    // Render Remote Plane (Shifted in X)
    if (m_remoteSRV) {
        context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        cb = (ConstantBuffer*)ms.pData;
        world = DirectX::XMMatrixTranslation(4.0f, 0.0f, 0.0f);
        cb->worldViewProj = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projMatrix);
        context->Unmap(m_constantBuffer, 0);
        context->PSSetShaderResources(0, 1, &m_remoteSRV);
        context->Draw(4, 0);
    }

    // Render Spatial Cursors
    for (auto const& [id, peer] : peers) {
        float cx = (float)peer.x / 1920.0f * 2.0f - 1.0f;
        float cy = 1.0f - (float)peer.y / 1080.0f * 2.0f;

        context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        cb = (ConstantBuffer*)ms.pData;
        world = DirectX::XMMatrixScaling(0.05f, 0.05f, 1.0f) * DirectX::XMMatrixTranslation(cx, cy, -0.01f);
        cb->worldViewProj = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projMatrix);
        context->Unmap(m_constantBuffer, 0);
        // Bind cursor texture and draw...
        context->Draw(4, 0);
    }

    // Render Webcam PiP (Local: Bottom-Right, Remote: Bottom-Left)
    if (m_localWebcamSRV) {
        context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        cb = (ConstantBuffer*)ms.pData;
        world = DirectX::XMMatrixScaling(0.2f, 0.2f, 1.0f) * DirectX::XMMatrixTranslation(0.75f, -0.75f, -0.02f);
        cb->worldViewProj = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projMatrix);
        context->Unmap(m_constantBuffer, 0);
        context->PSSetShaderResources(0, 1, &m_localWebcamSRV);
        context->Draw(4, 0);
    }

    if (m_remoteWebcamSRV) {
        context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        cb = (ConstantBuffer*)ms.pData;
        world = DirectX::XMMatrixScaling(0.2f, 0.2f, 1.0f) * DirectX::XMMatrixTranslation(-0.75f, -0.75f, -0.02f);
        cb->worldViewProj = DirectX::XMMatrixTranspose(world * m_viewMatrix * m_projMatrix);
        context->Unmap(m_constantBuffer, 0);
        context->PSSetShaderResources(0, 1, &m_remoteWebcamSRV);
        context->Draw(4, 0);
    }
}

void SpatialViewport::SetLocalDesktopTexture(ID3D11ShaderResourceView* srv) {
    if (m_localSRV) m_localSRV->Release();
    m_localSRV = srv;
    if (m_localSRV) m_localSRV->AddRef();
}

void SpatialViewport::SetRemoteDesktopTexture(ID3D11ShaderResourceView* srv) {
    if (m_remoteSRV) m_remoteSRV->Release();
    m_remoteSRV = srv;
    if (m_remoteSRV) m_remoteSRV->AddRef();
}

void SpatialViewport::SetLocalWebcamTexture(ID3D11ShaderResourceView* srv) {
    if (m_localWebcamSRV) m_localWebcamSRV->Release();
    m_localWebcamSRV = srv;
    if (m_localWebcamSRV) m_localWebcamSRV->AddRef();
}

void SpatialViewport::SetRemoteWebcamTexture(ID3D11ShaderResourceView* srv) {
    if (m_remoteWebcamSRV) m_remoteWebcamSRV->Release();
    m_remoteWebcamSRV = srv;
    if (m_remoteWebcamSRV) m_remoteWebcamSRV->AddRef();
}
#endif
