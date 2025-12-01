#pragma once

#include "GfxDevice.h"

struct Swapchain;
struct FrameSync
{
    u32 _frameIndex;
    HANDLE _fenceEvent;
    ComPtr<ID3D12Fence> _fence;
    u64 _fenceValues[2];

    ImmediateContext immediateContext;
};

FrameSync CreateFrameSyncResources(GfxDevice& gfxDevice);
void WaitForGPU(const GfxDevice& gfxDevice, FrameSync& frameSync);
void MoveToNextFrame(const GfxDevice& gfxDevice, const Swapchain& swapchain, FrameSync& frameSync);

void DestroyFrameSyncResources(GfxDevice& gfxDevice, FrameSync& frameSync);