#include "SpatialViewport.hpp"
#include "WebRTCManager.hpp"
#include <iostream>

SpatialViewport::SpatialViewport()
#ifdef _WIN32
    : m_currentCamX(0.0f), m_currentCamZ(0.0f), m_localSRV(nullptr), m_remoteSRV(nullptr),
      m_vertexShader(nullptr), m_pixelShader(nullptr), m_inputLayout(nullptr),
      m_vertexBuffer(nullptr), m_indexBuffer(nullptr), m_constantBuffer(nullptr),
      m_samplerState(nullptr)
#endif
{
}

SpatialViewport::~SpatialViewport() {
#ifdef _WIN32
    if (m_samplerState) m_samplerState->Release();
    if (m_constantBuffer) m_constantBuffer->Release();
    if (m_indexBuffer) m_indexBuffer->Release();
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_pixelShader) m_pixelShader->Release();
    if (m_vertexShader) m_vertexShader->Release();
#endif
}

#ifdef _WIN32
bool SpatialViewport::Initialize(ID3D11Device* device) {
    std::cout << "[SpatialViewport] Initializing 3D Composition Engine (D3D11)..." << std::endl;

    // Set up View and Projection matrices for 3D space
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    m_projMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.01f, 100.0f);

    if (!device) return false;

    // 1. Create Quad Vertices
    Vertex vertices[] = {
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) }, // Top-left
        { DirectX::XMFLOAT3( 1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) }, // Top-right
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) }, // Bottom-left
        { DirectX::XMFLOAT3( 1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }  // Bottom-right
    };

    D3D11_BUFFER_DESC vertexDesc = {};
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.ByteWidth = sizeof(Vertex) * 4;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices;
    device->CreateBuffer(&vertexDesc, &vertexData, &m_vertexBuffer);

    // 2. Create Quad Indices
    unsigned long indices[] = { 0, 1, 2, 2, 1, 3 };
    D3D11_BUFFER_DESC indexDesc = {};
    indexDesc.Usage = D3D11_USAGE_DEFAULT;
    indexDesc.ByteWidth = sizeof(unsigned long) * 6;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;
    device->CreateBuffer(&indexDesc, &indexData, &m_indexBuffer);

    // 3. Create Constant Buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(ConstantBufferType);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);

    // 4. Create Sampler State
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 8;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&samplerDesc, &m_samplerState);

    // Compile basic shaders
    const char* vertexShaderSrc =
        "cbuffer ConstantBuffer : register(b0) {"
        "    matrix World;"
        "    matrix View;"
        "    matrix Projection;"
        "}"
        "struct VS_INPUT {"
        "    float3 Pos : POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "struct PS_INPUT {"
        "    float4 Pos : SV_POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "PS_INPUT main(VS_INPUT input) {"
        "    PS_INPUT output = (PS_INPUT)0;"
        "    float4 pos = float4(input.Pos, 1.0f);"
        "    pos = mul(pos, World);"
        "    pos = mul(pos, View);"
        "    pos = mul(pos, Projection);"
        "    output.Pos = pos;"
        "    output.Tex = input.Tex;"
        "    return output;"
        "}";

    const char* pixelShaderSrc =
        "Texture2D txDiffuse : register(t0);"
        "SamplerState samLinear : register(s0);"
        "struct PS_INPUT {"
        "    float4 Pos : SV_POSITION;"
        "    float2 Tex : TEXCOORD0;"
        "};"
        "float4 main(PS_INPUT input) : SV_Target {"
        "    return txDiffuse.Sample(samLinear, input.Tex);"
        "}";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(vertexShaderSrc, strlen(vertexShaderSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
        vsBlob->Release();
    } else {
        if (errorBlob) {
            std::cerr << "[SpatialViewport] VS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
    }

    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(pixelShaderSrc, strlen(pixelShaderSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
        psBlob->Release();
    } else {
        if (errorBlob) {
            std::cerr << "[SpatialViewport] PS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
    }

    return true;
}

void SpatialViewport::Update(float deltaTime, bool isCrossingBorder) {
    // Boundary-triggered Spatial Animation (The "Zoom")
    // Enhance smoothing logic to make it more seamless
    float targetX = isCrossingBorder ? 2.5f : 0.0f; // Shift slightly towards the remote plane
    float targetZ = isCrossingBorder ? -6.0f : -4.0f; // Zoom out more for context

    // Smooth step easing (could use smoothstep or similar for better feel)
    float lerpSpeed = 8.0f; // Slightly faster, but smoother easing

    // Simple exponential smoothing
    m_currentCamX += (targetX - m_currentCamX) * (1.0f - std::exp(-lerpSpeed * deltaTime));
    m_currentCamZ += (targetZ - m_currentCamZ) * (1.0f - std::exp(-lerpSpeed * deltaTime));

    // Update view matrix based on interpolated camera position
    // Adding a slight tilt to the camera for a 3D effect
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(m_currentCamX, 0.5f, m_currentCamZ, 0.0f); // Slightly elevated
    DirectX::XMVECTOR At = DirectX::XMVectorSet(m_currentCamX * 0.5f, 0.0f, 0.0f, 0.0f);  // Look slightly towards center
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(Eye, At, Up);
}

void SpatialViewport::Render(ID3D11DeviceContext* context, const std::map<unsigned long long, RemoteCursorState>& peers, float localDpiScale) {
    if (!context) return;

    // Set buffers and topology
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set shaders and sampler (assuming they were created)
    if (m_vertexShader) context->VSSetShader(m_vertexShader, nullptr, 0);
    if (m_pixelShader) context->PSSetShader(m_pixelShader, nullptr, 0);
    if (m_samplerState) context->PSSetSamplers(0, 1, &m_samplerState);

    // 1. Draw Local Desktop Plane
    if (m_localSRV) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            ConstantBufferType* dataPtr = (ConstantBufferType*)mappedResource.pData;

            // Local plane is centered at world origin
            dataPtr->world = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
            dataPtr->view = DirectX::XMMatrixTranspose(m_viewMatrix);
            dataPtr->projection = DirectX::XMMatrixTranspose(m_projMatrix);

            context->Unmap(m_constantBuffer, 0);
            context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
        }

        context->PSSetShaderResources(0, 1, &m_localSRV);
        context->DrawIndexed(6, 0, 0);
    }

    // 2. Draw Remote Desktop Planes
    int planeIndex = 1;
    for (auto const& [id, peer] : peers) {
        // Obtain remote SRV from WebRTC media pipeline manager
        ID3D11ShaderResourceView* remoteSRV = WebRTCManager::GetInstance()->GetRemoteDesktopTexture(id);

        if (remoteSRV) {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                ConstantBufferType* dataPtr = (ConstantBufferType*)mappedResource.pData;

                // Offset each remote plane in X space
                DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixTranslation(planeIndex * 2.5f, 0.0f, 0.0f);

                dataPtr->world = DirectX::XMMatrixTranspose(worldMatrix);
                dataPtr->view = DirectX::XMMatrixTranspose(m_viewMatrix);
                dataPtr->projection = DirectX::XMMatrixTranspose(m_projMatrix);

                context->Unmap(m_constantBuffer, 0);
                context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
            }

            context->PSSetShaderResources(0, 1, &remoteSRV);
            context->DrawIndexed(6, 0, 0);
        }

        planeIndex++;
    }
}

void SpatialViewport::UpdateLocalDesktopFrame(ID3D11Texture2D* frame, ID3D11Device* device) {
    if (!frame || !device) return;

    if (m_localSRV) {
        m_localSRV->Release();
        m_localSRV = nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(frame, &srvDesc, &m_localSRV);
}

#endif
