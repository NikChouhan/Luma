#include "FrameSync.h"
#include "Swapchain.h"

FrameSync CreateFrameSyncResources(GfxDevice& gfxDevice)
{
    FrameSync frameSync{};

    DX_ASSERT(gfxDevice._device->CreateFence(frameSync._fenceValues[frameSync._frameIndex],
        D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameSync._fence)));
    frameSync._fenceValues[frameSync._frameIndex]++;

    // create event handle to use for frame sync
    frameSync._fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (frameSync._fenceEvent == nullptr)
    {
        DX_ASSERT(HRESULT_FROM_WIN32(GetLastError()));
    }
    return frameSync;
}

void WaitForGPU(GfxDevice& gfxDevice, FrameSync& frameSync)
{
    /* commandQueue->Signal processes the GPU side command to set the fence to the fence value at the particular index
     * then fence->SetEventOnCompletion sets the event the same (set the fence value to frame index)
     * but on the CPU. The next command (WaitForSingleObjectEx) waits for the fence event CPU side to complete in the GPU side.
     * We basically linked the GPU and the CPU side with an event and a fence. Then increase the fence value for the next frame
    */

    /* This is more imp info:
    * what Singal command does is it pushes the value of the fenceValue (for the particular frame index) at the end of the 
    * command queue. This ensures that all the commands submitted prior execute and then the value of the fence is set to
    * the submitted fence value
    */
    
    // schedule signal command in the queue
    DX_ASSERT(gfxDevice._commandQueue->Signal(frameSync._fence.Get(), frameSync._fenceValues[frameSync._frameIndex]));

    // wait until the fence has been processed
    /* the fence->SetEventOnCompletion fn checks the value of the fence with the fenceValue and stalls the CPU main thread
    * until the value is reached
    */
    DX_ASSERT(frameSync._fence->SetEventOnCompletion(frameSync._fenceValues[frameSync._frameIndex], frameSync._fenceEvent));
    WaitForSingleObjectEx(frameSync._fenceEvent, INFINITE, FALSE);

    // increment the fence value for the current frame
    frameSync._fenceValues[frameSync._frameIndex]++;
}

void MoveToNextFrame(GfxDevice& gfxDevice, Swapchain& swapchain, FrameSync& frameSync)  
{
    // schedule a Signal command in the queue
    const u64 currentFenceValue = frameSync._fenceValues[frameSync._frameIndex];
    DX_ASSERT(gfxDevice._commandQueue->Signal(frameSync._fence.Get(), currentFenceValue));

    // update the frame index
    frameSync._frameIndex = swapchain._swapchain->GetCurrentBackBufferIndex();
    
    // CPU side code
    // if the next frame is not ready to be rendered yet, wait until its ready
    if (frameSync._fence->GetCompletedValue() < frameSync._fenceValues[frameSync._frameIndex])
    {
        DX_ASSERT(frameSync._fence->SetEventOnCompletion(frameSync._fenceValues[frameSync._frameIndex], frameSync._fenceEvent));
        WaitForSingleObjectEx(frameSync._fenceEvent, INFINITE, FALSE);
    }

    // set the fence value for the next frame
    frameSync._fenceValues[frameSync._frameIndex] = currentFenceValue + 1;
}

