#pragma once

#include "GfxDevice.h"

struct Camera;
struct Model;
struct Texture;
struct Pipeline;
struct FrameSync;
struct Buffer;

struct Swapchain
{
    CD3DX12_VIEWPORT _viewport;
    CD3DX12_RECT _scissorRect;
    ComPtr<IDXGISwapChain4> _swapchain;
    // rtv resources
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12Resource> _renderTargets[frameCount];
    u32 _rtvDescriptorSize{0};
    // dsv resources
    ComPtr<ID3D12DescriptorHeap> _dsvHeap;
    ComPtr<ID3D12Resource> _depthStencil;
    u32 _dsvDescriptorSize{0};
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

Swapchain CreatSwapChain(GfxDevice& gfxDevice, FrameSync& frameSync, SwapchainDesc desc);
void DestroySwapChain(Swapchain& swapchain);

void SubmitandPresent(ComPtr<ID3D12GraphicsCommandList> commandList, GfxDevice& gfxDevice,
    Swapchain& swapchain, FrameSync& frameSync, Camera& camera, Pipeline& pipeline, Model& model);