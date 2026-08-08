#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <cstdint>

class WallpaperRenderer {
private:
    HWND m_hwnd = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
    ID3D11Texture2D* m_wallpaperTexture = nullptr;
    ID3D11ShaderResourceView* m_srv = nullptr;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;
    D3D11_VIEWPORT m_viewport = {};
    bool m_isInitialized = false;

    WallpaperRenderer() = default;

    void CreateShaders();

public:
    static WallpaperRenderer& GetInstance();

    bool Initialize(HWND hwnd);
    void Render();
    void UpdateTexture(const uint8_t* data, int width, int height);
    void Shutdown();

    bool IsInitialized() const { return m_isInitialized; }
};
