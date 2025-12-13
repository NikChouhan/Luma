#pragma once

#include "Graphics/GfxDevice.h"
#include <vector>

struct CommandList;
struct Inspector;
struct Camera;
struct Model;
struct Texture;
struct Pipeline;
struct FrameSync;

struct Swapchain
{
    GfxDevice* gfxDevice_;
    FrameSync* frameSync_;

    CD3DX12_VIEWPORT viewport_;
    CD3DX12_RECT scissorRect_;
    ComPtr<IDXGISwapChain4> swapchain_;
    // rtv resources
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    ComPtr<ID3D12Resource> renderTargets_[frameCount];
    u32 rtvDescriptorSize_{0};
    // dsv resources
    ComPtr<ID3D12DescriptorHeap> dsvDepthHeap_;
    ComPtr<ID3D12Resource> depthStencil_;
	u32 dsvDescriptorSize_{0};

    // compute shader background effects resource
    ComPtr<ID3D12Resource> uavBgShaderEffects_;

    bool isResizing_ = false;
    bool needsResize_ = false;

    HWND hwnd_;

    u16 height_{};
    u16 width_{};

    void ResizeSwapChain(u16 width, u16 height, Model* model);
};
struct SwapchainDesc
{
    u16 height_{};
    u16 width_{};
    bool vsyncEnable_{};
    HWND hwnd_ = nullptr;
};

Swapchain CreateSwapChain(GfxDevice& gfxDevice, FrameSync& frameSync, SwapchainDesc desc);
void DestroySwapChain(Swapchain& swapchain);