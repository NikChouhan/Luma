#include "Graphics/D3D12/Swapchain.h"

#include <imgui_internal.h>

#include "Graphics/D3D12/Buffer.h"
#include "Graphics/FrameSync.h"
#include "Renderer/Model.h"
#include "Graphics/D3D12/Pipeline.h"
#include "Graphics/D3D12/Texture.h"
#include "Core/Camera.h"
#include "Graphics/FrameSync.h"
#include "Graphics/PushConstants.h"

#include "Renderer/Core/Inspector.h"

#define IF_DEPTH 1

Swapchain CreateSwapChain(GfxDevice& gfxDevice, FrameSync& frameSync, SwapchainDesc desc)
{
    Swapchain swapchain{};

    swapchain.gfxDevice_ = &gfxDevice;
    swapchain.frameSync_ = &frameSync;

    swapchain.hwnd_ = desc.hwnd_;
    // viewport and scissor
    swapchain.viewport_.TopLeftX = 0;
    swapchain.viewport_.TopLeftY = 0;
    swapchain.viewport_.Height = desc.height_;
    swapchain.viewport_.Width = desc.width_;
    swapchain.viewport_.MinDepth = 0.0f;
    swapchain.viewport_.MaxDepth = 1.0f;

    swapchain.height_ = desc.height_;
    swapchain.width_ = desc.width_;

    swapchain.scissorRect_.left = 0;
    swapchain.scissorRect_.top = 0;
    swapchain.scissorRect_.right = static_cast<LONG>(desc.width_);
    swapchain.scissorRect_.bottom = static_cast<LONG>(desc.height_);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = frameCount;
    
    swapChainDesc.Width = static_cast<u32>(swapchain.viewport_.Width);
    swapChainDesc.Height = static_cast<u32>(swapchain.viewport_.Height);
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    DX_ASSERT(gfxDevice.factory_->CreateSwapChainForHwnd(
        gfxDevice.commandQueue_.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        swapchain.hwnd_,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    DX_ASSERT(gfxDevice.factory_->MakeWindowAssociation(swapchain.hwnd_, DXGI_MWA_NO_ALT_ENTER));
    DX_ASSERT(swapChain.As(&swapchain.swapchain_));

    // create descriptor heaps
    {
	    // render target heap
	    {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
            rtvHeapDesc.NumDescriptors = frameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            DX_ASSERT(gfxDevice.device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&swapchain.rtvHeap_)));
	    }

	    swapchain.rtvDescriptorSize_ = gfxDevice.device_->GetDescriptorHandleIncrementSize(
		    D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	    // depth buffer DSV heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DX_ASSERT(gfxDevice.device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&swapchain.dsvDepthHeap_)));
    }

    // create frame resources
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain.rtvHeap_->GetCPUDescriptorHandleForHeapStart());

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        // create rtv for each frame
        for (u32 i = 0; i< frameCount; i++)
        {
            DX_ASSERT(swapchain.swapchain_->GetBuffer(i, IID_PPV_ARGS(&swapchain.renderTargets_[i])));
            gfxDevice.device_->CreateRenderTargetView(swapchain.renderTargets_[i].Get(),
                &rtvDesc, rtvHandle);
            rtvHandle.Offset(1, swapchain.rtvDescriptorSize_);
        }
    }
    // create depth stencil view
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
        depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimisedClearValue{};
        depthOptimisedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimisedClearValue.DepthStencil.Depth = 0.f;
        depthOptimisedClearValue.DepthStencil.Stencil = 0;

        const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
            u64(desc.width_), u64(desc.height_),
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        DX_ASSERT(gfxDevice.device_->CreateCommittedResource(
            &depthStencilHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimisedClearValue,
            IID_PPV_ARGS(&swapchain.depthStencil_)));

        gfxDevice.device_->CreateDepthStencilView(swapchain.depthStencil_.Get(), &depthStencilViewDesc,
            swapchain.dsvDepthHeap_->GetCPUDescriptorHandleForHeapStart());
    }
    return swapchain;
}