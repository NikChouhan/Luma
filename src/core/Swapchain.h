#pragma once

#include "GfxDevice.h"

struct Inspector;
struct Camera;
struct Model;
struct Texture;
struct Pipeline;
struct FrameSync;

struct Swapchain
{
    GfxDevice* _gfxDevice;
    FrameSync* _frameSync;

    CD3DX12_VIEWPORT _viewport;
    CD3DX12_RECT _scissorRect;
    ComPtr<IDXGISwapChain4> _swapchain;
    // rtv resources
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12Resource> _renderTargets[frameCount];
    u32 _rtvDescriptorSize{0};
    // dsv resources
    ComPtr<ID3D12DescriptorHeap> _dsvDepthHeap;
    ComPtr<ID3D12DescriptorHeap> _srvDepthHeap;
    ComPtr<ID3D12Resource> _depthStencil;
	u32 _dsvDescriptorSize{0};

    // compute shader background effects resource
    ComPtr<ID3D12Resource> _uavBgShaderEffects;

    bool _isResizing = false;
    bool _needsResize = false;

    HWND _hwnd;

    u16 _height{};
    u16 _width{};

    void ResizeSwapChain(u16 width, u16 height, Model* model);
};
struct SwapchainDesc
{
    u16 _height{};
    u16 _width{};
    bool _vsyncEnable{};
    HWND _hwnd = nullptr;
};

Swapchain CreateSwapChain(GfxDevice& gfxDevice, FrameSync& frameSync, SwapchainDesc desc);
void DestroySwapChain(Swapchain& swapchain);

void SubmitPasses(ComPtr<ID3D12GraphicsCommandList> commandList, GfxDevice& gfxDevice,
    Swapchain& swapchain, FrameSync& frameSync, Inspector& inspector, Camera& camera, Pipeline& backdropComputePipeline,
    Pipeline& depthPassPipeline, Pipeline& rasterPipeline, Model& model);

void SetupLightSettingsHandler();