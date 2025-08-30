#include "Swapchain.h"

#include "Buffer.h"
#include "FrameSync.h"
#include "Pipeline.h"

Swapchain CreatSwapChain(GfxDevice& gfxDevice, SwapchainDesc desc)
{
    Swapchain swapchain{};

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = frameCount;
    swapChainDesc.Width = static_cast<u32>(gfxDevice._viewport.Width);
    swapChainDesc.Height = static_cast<u32>(gfxDevice._viewport.Height);
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    DX_ASSERT(gfxDevice._factory->CreateSwapChainForHwnd(
        gfxDevice._commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        gfxDevice._hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    DX_ASSERT(gfxDevice._factory->MakeWindowAssociation(gfxDevice._hwnd, DXGI_MWA_NO_ALT_ENTER));
    DX_ASSERT(swapChain.As(&swapchain._swapchain));

    // create descriptor heaps
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DX_ASSERT(gfxDevice._device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&swapchain._rtvHeap)));

        swapchain._rtvDescriptorSize = gfxDevice._device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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

    return swapchain;
}

void SubmitandPresent(ComPtr<ID3D12GraphicsCommandList> commandList,
                      GfxDevice& gfxDevice,
                      Swapchain& swapchain,
                      FrameSync& frameSync,
                      Pipeline& pipeline,
                      Buffer& vertexBuffer)
{
    // record command list
    {
        DX_ASSERT(gfxDevice._commandAllocators[frameSync._frameIndex]->Reset());
        DX_ASSERT(commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(),
            pipeline._pipelineState.Get()));
        commandList->SetGraphicsRootSignature(pipeline._rootSignature.Get());
        commandList->RSSetViewports(1, &gfxDevice._viewport);
        commandList->RSSetScissorRects(1, &gfxDevice._scissorRect);

        // set the back buffer as render target
        auto rBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &rBarrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain._rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameSync._frameIndex, swapchain._rtvDescriptorSize);
        commandList->OMSetRenderTargets(1, &rtvHandle,
            FALSE,nullptr);

        // record commands
        const float clearColor[4] = {0.1f, 0.2f, 0.4f, 1.0f};
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBuffer._bufferView);
        commandList->DrawInstanced(3, 1, 0, 0);

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








































