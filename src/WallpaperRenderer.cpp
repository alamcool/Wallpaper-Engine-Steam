#include "WallpaperRenderer.h"
#include "Config.h"
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

WallpaperRenderer& WallpaperRenderer::GetInstance() {
    static WallpaperRenderer instance;
    return instance;
}

bool WallpaperRenderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = 0;
    scd.BufferDesc.Height = 0;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
        &scd, &m_swapChain, &m_device, nullptr, &m_context
    );

    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
            &scd, &m_swapChain, &m_device, nullptr, &m_context
        );
    }

    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = GetSystemMetrics(SM_CXSCREEN);
    texDesc.Height = GetSystemMetrics(SM_CYSCREEN);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DYNAMIC;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    m_device->CreateTexture2D(&texDesc, nullptr, &m_wallpaperTexture);

    m_viewport.Width = (float)texDesc.Width;
    m_viewport.Height = (float)texDesc.Height;
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    CreateShaders();

    m_isInitialized = true;
    return true;
}

void WallpaperRenderer::CreateShaders() {
    const char* vsSrc =
        "struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };"
        "VSOut main(float2 pos : POSITION, float2 uv : TEXCOORD0) {"
        "  VSOut o; o.pos = float4(pos, 0.0, 1.0); o.uv = uv; return o;"
        "}";

    const char* psSrc =
        "Texture2D tex : register(t0);"
        "SamplerState samp : register(s0);"
        "float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {"
        "  return tex.Sample(samp, uv);"
        "}";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    D3DCompile(vsSrc, strlen(vsSrc), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(psSrc, strlen(psSrc), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, nullptr);

    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    m_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();
    psBlob->Release();

    float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    m_device->CreateBuffer(&bd, &initData, &m_vertexBuffer);

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    m_device->CreateSamplerState(&sampDesc, &m_samplerState);

    m_device->CreateShaderResourceView(m_wallpaperTexture, nullptr, &m_srv);
}

void WallpaperRenderer::Render() {
    if (!m_isInitialized) return;

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_context->ClearRenderTargetView(m_rtv, clearColor);

    m_context->RSSetViewports(1, &m_viewport);
    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);

    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &m_srv);
    m_context->PSSetSamplers(0, 1, &m_samplerState);

    UINT stride = 16;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    m_context->Draw(4, 0);

    m_swapChain->Present(1, 0);
}

void WallpaperRenderer::UpdateTexture(const uint8_t* data, int width, int height) {
    if (!m_wallpaperTexture || !data) return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_context->Map(m_wallpaperTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        int srcPitch = width * 4;
        uint8_t* dst = (uint8_t*)mapped.pData;
        for (int y = 0; y < height; y++) {
            memcpy(dst + y * mapped.RowPitch, data + y * srcPitch, srcPitch);
        }
        m_context->Unmap(m_wallpaperTexture, 0);
    }
}

void WallpaperRenderer::Shutdown() {
    if (m_srv) { m_srv->Release(); m_srv = nullptr; }
    if (m_samplerState) { m_samplerState->Release(); m_samplerState = nullptr; }
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = nullptr; }
    if (m_inputLayout) { m_inputLayout->Release(); m_inputLayout = nullptr; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = nullptr; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = nullptr; }
    if (m_wallpaperTexture) { m_wallpaperTexture->Release(); m_wallpaperTexture = nullptr; }
    if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
    m_isInitialized = false;
}
