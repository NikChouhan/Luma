#pragma once

#include "GfxDevice.h"

struct Pipeline;
struct FrameSync;
struct Buffer;

struct Swapchain
{
    ComPtr<IDXGISwapChain4> _swapchain;
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12Resource> _renderTargets[frameCount];
    u32 _rtvDescriptorSize{0};
};
struct SwapchainDesc
{
    bool _vsyncEnable;
};

Swapchain CreatSwapChain(GfxDevice& gfxDevice, SwapchainDesc desc);
void DestroySwapChain(Swapchain& swapchain);

void SubmitandPresent(ComPtr<ID3D12GraphicsCommandList> commandList, GfxDevice& gfxDevice,
    Swapchain& swapchain, FrameSync& frameSync, Pipeline& pipeline, Buffer& vertexBuffer);