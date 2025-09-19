#include "Swapchain.h"

#include "Buffer.h"
#include "FrameSync.h"
#include "Model.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Camera.h"

Swapchain CreatSwapChain(GfxDevice& gfxDevice, FrameSync& frameSync, SwapchainDesc desc)
{
    Swapchain swapchain{};

    swapchain._aspectRatio = desc._aspectRatio;
    swapchain._hwnd = desc._hwnd;
    // viewport and scissor
    swapchain._viewport.TopLeftX = 0;
    swapchain._viewport.TopLeftY = 0;
    swapchain._viewport.Height = static_cast<float>(desc._height);
    swapchain._viewport.Width = static_cast<float>(desc._width);

    swapchain._scissorRect.left = 0;
    swapchain._scissorRect.top = 0;
    swapchain._scissorRect.right = static_cast<LONG>(desc._width);
    swapchain._scissorRect.bottom = static_cast<LONG>(desc._height);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = frameCount;
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
	    // depth stencil heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DX_ASSERT(gfxDevice._device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&swapchain._dsvHeap)));

        swapchain._dsvDescriptorSize = gfxDevice._device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
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
            swapchain._dsvHeap->GetCPUDescriptorHandleForHeapStart());


        // resource state change can be done like shown below, but I avoid it here, since depth stencil is already
        // init in depth write state

        /*ImmediateSubmit(gfxDevice, frameSync, [&](ComPtr<ID3D12GraphicsCommandList1> commandList)
            {
                CD3DX12_RESOURCE_BARRIER pRBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain._depthStencil.Get(),
                    D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                commandList->ResourceBarrier(1, &pRBarrier);
            });*/
    }

    return swapchain;
}


void SubmitandPresent(ComPtr<ID3D12GraphicsCommandList> commandList,
    GfxDevice& gfxDevice,
    Swapchain& swapchain,
    FrameSync& frameSync,
    Camera& camera,
    Pipeline& pipeline,
    Model& model)
{
    // record command list
    {
        WaitForGPU(gfxDevice, frameSync);
        DX_ASSERT(gfxDevice._commandAllocators[frameSync._frameIndex]->Reset());
        DX_ASSERT(commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(),
            pipeline._pipelineState.Get()));

        ID3D12DescriptorHeap* ppHeaps[] = { model._modelHeap.Get() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        commandList->SetGraphicsRootSignature(pipeline._rootSignature.Get());

        commandList->RSSetViewports(1, &swapchain._viewport);
        commandList->RSSetScissorRects(1, &swapchain._scissorRect);

        // set the back buffer as render target
        auto rBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &rBarrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain._rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            frameSync._frameIndex, swapchain._rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(swapchain._dsvHeap->GetCPUDescriptorHandleForHeapStart(),
            0, swapchain._dsvDescriptorSize);
        commandList->OMSetRenderTargets(1, &rtvHandle,
            FALSE,&dsvHandle);

        // record commands
        const float clearColor[4] = {0.1f, 0.2f, 0.4f, 1.0f};
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, &swapchain._scissorRect);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->IASetVertexBuffers(0, 1, &model._vertexBuffer._vertexBufferView);
    	commandList->IASetIndexBuffer(&model._indexBuffer._indexBufferView);

        /*CD3DX12_GPU_DESCRIPTOR_HANDLE texGPUHandle(model._modelHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->SetGraphicsRootDescriptorTable(1, texGPUHandle);*/

        for (auto& mesh : model)
        {
            Material& currentMaterial = model._materials[mesh._materialIndex];
            XMMATRIX world = mesh._transform._matrix;
            XMMATRIX view = camera._view;
            XMMATRIX proj = camera._projection;

            XMMATRIX worldViewProj = world * view * proj;
            ConstBuffer pushConstants{};
            pushConstants._materialIndex = currentMaterial._albedoIndex;    
            pushConstants._worldViewProj = worldViewProj;
            pushConstants._worldMatrix = world;
            commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstBuffer)/4, &pushConstants, 0);

			commandList->DrawIndexedInstanced(mesh._indexCount,
                1, mesh._startIndex, mesh._startVertex, 0);
        }
        

        // transition the render target to present format
        rBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &rBarrier);


        DX_ASSERT(commandList->Close());
    }

    // execute the command list
    ID3D12CommandList* ppCommandLists[] = {commandList.Get()};
    gfxDevice._commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // present the Frame
    DX_ASSERT(swapchain._swapchain->Present(1,0));
}








































