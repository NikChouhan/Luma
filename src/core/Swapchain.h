#pragma once

#include "GfxDevice.h"

struct Texture;
struct Pipeline;
struct FrameSync;
struct Buffer;

struct Swapchain
{
    CD3DX12_VIEWPORT _viewport;
    CD3DX12_RECT _scissorRect;
    ComPtr<IDXGISwapChain4> _swapchain;
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12Resource> _renderTargets[frameCount];
    u32 _rtvDescriptorSize{0};
    HWND _hwnd;
    float _aspectRatio{};
    
};
struct SwapchainDesc
{
    float _aspectRatio{};
    float _height{};
    float _width{};
    bool _vsyncEnable{};
    HWND _hwnd = nullptr;
};

Swapchain CreatSwapChain(GfxDevice& gfxDevice, SwapchainDesc desc);
void DestroySwapChain(Swapchain& swapchain);

void SubmitandPresent(ComPtr<ID3D12GraphicsCommandList> commandList, GfxDevice& gfxDevice,
    Swapchain& swapchain, FrameSync& frameSync, Pipeline& pipeline, Buffer& vertexBuffer, Texture& texture);