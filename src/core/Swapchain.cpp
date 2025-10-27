#include "Swapchain.h"

#include "Buffer.h"
#include "FrameSync.h"
#include "Model.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Camera.h"

#define IF_DEPTH 1

Swapchain CreatSwapChain(GfxDevice& gfxDevice, FrameSync& frameSync, SwapchainDesc desc)
{
    Swapchain swapchain{};

    swapchain._aspectRatio = desc._aspectRatio;
    swapchain._hwnd = desc._hwnd;
    // viewport and scissor
    swapchain._viewport.TopLeftX = 0;
    swapchain._viewport.TopLeftY = 0;
    swapchain._viewport.Height = desc._height;
    swapchain._viewport.Width = desc._width;
    swapchain._viewport.MinDepth = 1.0f;
    swapchain._viewport.MaxDepth = 0.0f;

    swapchain._scissorRect.left = 0;
    swapchain._scissorRect.top = 0;
    swapchain._scissorRect.right = static_cast<LONG>(desc._width);
    swapchain._scissorRect.bottom = static_cast<LONG>(desc._height);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = frameCount;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    swapChainDesc.Width = static_cast<u32>(swapchain._viewport.Width);
    swapChainDesc.Height = static_cast<u32>(swapchain._viewport.Height);
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    DX_ASSERT(gfxDevice._factory->CreateSwapChainForHwnd(
        gfxDevice._commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        swapchain._hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    DX_ASSERT(gfxDevice._factory->MakeWindowAssociation(swapchain._hwnd, DXGI_MWA_NO_ALT_ENTER));
    DX_ASSERT(swapChain.As(&swapchain._swapchain));

    // create descriptor heaps
    {
	    // render target heap
	    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	    rtvHeapDesc.NumDescriptors = frameCount;
	    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	    DX_ASSERT(gfxDevice._device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&swapchain._rtvHeap)));

	    swapchain._rtvDescriptorSize = gfxDevice._device->GetDescriptorHandleIncrementSize(
		    D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	    // depth buffer DSV heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DX_ASSERT(gfxDevice._device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&swapchain._dsvDepthHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        gfxDevice._device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&swapchain._srvDepthHeap));
    }

    // create frame resources
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain._rtvHeap->GetCPUDescriptorHandleForHeapStart());
        // create rtv for each frame
        for (u32 i = 0; i< frameCount; i++)
        {
            DX_ASSERT(swapchain._swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain._renderTargets[i])));
            gfxDevice._device->CreateRenderTargetView(swapchain._renderTargets[i].Get(),
                nullptr, rtvHandle);
            rtvHandle.Offset(1, swapchain._rtvDescriptorSize);
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
        depthOptimisedClearValue.DepthStencil.Depth = 1.f;
        depthOptimisedClearValue.DepthStencil.Stencil = 0;

        const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
            u64(desc._width), u64(desc._height),
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        DX_ASSERT(gfxDevice._device->CreateCommittedResource(
            &depthStencilHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimisedClearValue,
            IID_PPV_ARGS(&swapchain._depthStencil)));

        gfxDevice._device->CreateDepthStencilView(swapchain._depthStencil.Get(), &depthStencilViewDesc,
            swapchain._dsvDepthHeap->GetCPUDescriptorHandleForHeapStart());

        D3D12_SHADER_RESOURCE_VIEW_DESC depthSRV{};
        depthSRV.Format = DXGI_FORMAT_R32_FLOAT;
        depthSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthSRV.Texture2D.MipLevels = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =  swapchain._srvDepthHeap->GetCPUDescriptorHandleForHeapStart();
        gfxDevice._device->CreateShaderResourceView(swapchain._depthStencil.Get(), &depthSRV, srvHandle);
    }

    return swapchain;
}


void SubmitPass(ComPtr<ID3D12GraphicsCommandList> commandList,
    GfxDevice& gfxDevice,
    Swapchain& swapchain,
    FrameSync& frameSync,
    Camera& camera,
    Pipeline& depthPassPipeline,
    Pipeline& rasterPipeline,
    Model& model)
{
#ifdef IF_DEPTH
    // depth pre-pass
	{
        WaitForGPU(gfxDevice, frameSync);
        DX_ASSERT(gfxDevice._commandAllocators[frameSync._frameIndex]->Reset());
        DX_ASSERT(commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(), 
            depthPassPipeline._pipelineState.Get()));

        commandList->SetGraphicsRootSignature(depthPassPipeline._rootSignature.Get());

        commandList->RSSetViewports(1, &swapchain._viewport);
        commandList->RSSetScissorRects(1, &swapchain._scissorRect);

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(swapchain._dsvDepthHeap->GetCPUDescriptorHandleForHeapStart());

	    // record commands
	    const float clearColor[4] = {0.f, 0., 1.f, 1.0f};

	    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	    commandList->OMSetRenderTargets(0, nullptr,
	       FALSE, &dsvHandle);
	    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	    commandList->IASetVertexBuffers(0, 1, &model._vertexBuffer.vertex_buffer_view);
		commandList->IASetIndexBuffer(&model._indexBuffer.index_buffer_view);

        for (auto& mesh : model)
        {
            XMMATRIX world = mesh._transform._matrix;

            //world = XMMatrixRotationX(DirectX::XM_PI) * world;

            XMMATRIX view = camera._view;
            XMMATRIX proj = camera._projection;

            XMMATRIX worldViewProj = world * view * proj;
            DepthPPBuffer pushConstants{};
            pushConstants._worldViewProj = worldViewProj;
            pushConstants._worldMatrix = (world);
            commandList->SetGraphicsRoot32BitConstants(0, sizeof(DepthPPBuffer) / 4, &pushConstants, 0);

            commandList->DrawIndexedInstanced(mesh._indexCount,
                1, mesh._startIndex, mesh._startVertex, 0);
        }
    }

    // ray tracing lights compute pass (delegated, going full forward with two stage - depth pre pass and then full pipeline
	/*{
        WaitForGPU(gfxDevice, frameSync);
        DX_ASSERT(commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(),
            rtPipeline._pipelineState.Get()));

        ID3D12DescriptorHeap* ppHeaps[] = { swapchain._srvDepthHeap.Get(), {model.uavHeapRT.Get()} };

		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		commandList->SetComputeRootSignature(rtPipeline._rootSignature.Get());

        commandList->RSSetViewports(1, &swapchain._viewport);
        commandList->RSSetScissorRects(1, &swapchain._scissorRect);

        auto rBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain._depthStencil.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        RTBuffer pushConstants{};
        commandList->SetComputeRoot32BitConstants(0, sizeof(RTBuffer) / 4, &pushConstants, 0);

        uint32_t dispatchX = (2560 + 7) / 8;
        uint32_t dispatchY = (1440 + 7) / 8;
        commandList->Dispatch(dispatchX, dispatchY, 1);
	}*/
    // forward RT path
    {
        commandList->SetPipelineState(rasterPipeline._pipelineState.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { model._commonHeap.Get() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        commandList->SetGraphicsRootSignature(rasterPipeline._rootSignature.Get());

        commandList->RSSetViewports(1, &swapchain._viewport);
        commandList->RSSetScissorRects(1, &swapchain._scissorRect);

        // set the back buffer as render target
        CD3DX12_RESOURCE_BARRIER rBarriers[1] = { CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
        };
        commandList->ResourceBarrier(1, rBarriers);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain._rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            frameSync._frameIndex, swapchain._rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(swapchain._dsvDepthHeap->GetCPUDescriptorHandleForHeapStart());

        // record commands
        const float clearColor[4] = {0.f, 0., 1.f, 1.0f};
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        commandList->OMSetRenderTargets(1, &rtvHandle,
            FALSE, &dsvHandle);

        for (auto& mesh : model)
        {
            Material& currentMaterial = model._materials[mesh._materialIndex];
            XMMATRIX world = mesh._transform._matrix;

            //world = XMMatrixRotationX(DirectX::XM_PI) * world;

            XMMATRIX view = camera._view;
            XMMATRIX proj = camera._projection;

            XMMATRIX worldViewProj = world * view * proj;
            ConstBuffer pushConstants{};
            pushConstants._albedoIndex = currentMaterial._albedoIndex;
            pushConstants._normalIndex = currentMaterial._normalIndex;
            pushConstants._accelerationStructureIndex = model._modelTextures.size();
            pushConstants._worldViewProj = worldViewProj;
            pushConstants._worldMatrix = (world);
            pushConstants._lightDir = camera._pos;
            pushConstants._cameraPos = camera._pos;
            commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstBuffer)/4, &pushConstants, 0);

			commandList->DrawIndexedInstanced(mesh._indexCount,
                1, mesh._startIndex, mesh._startVertex, 0);
        }

        // transition the render target to present format
        rBarriers[1] = { CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT) };
        commandList->ResourceBarrier(1, rBarriers);

        DX_ASSERT(commandList->Close());
        ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
        gfxDevice._commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }
#endif
}