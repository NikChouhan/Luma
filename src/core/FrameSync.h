#pragma once

#include "GfxDevice.h"

struct Swapchain;
struct FrameSync
{
    u32 _frameIndex;
    HANDLE _fenceEvent;
    ComPtr<ID3D12Fence> _fence;
    u64 _fenceValues[2];
};

FrameSync CreateFrameSyncResources(GfxDevice& gfxDevice);
void WaitForGPU(GfxDevice& gfxDevice, FrameSync& frameSync);
void MoveToNextFrame(GfxDevice& gfxDevice, Swapchain& swapchain, FrameSync& frameSync);

void DestroyFrameSyncResources(GfxDevice& gfxDevice, FrameSync& frameSync);