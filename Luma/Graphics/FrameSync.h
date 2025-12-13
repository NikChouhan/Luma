#pragma once

#include "Graphics/GfxDevice.h"
#include "Graphics/ImmediateContext.h"

struct Swapchain;
struct FrameSync
{
    u32 frameIndex_;
    HANDLE fenceEvent_;
    ComPtr<ID3D12Fence> fence_;
    u64 fenceValues_[2];

    ImmediateContext immediateContext_;
};

FrameSync CreateFrameSyncResources(GfxDevice& gfxDevice);
void WaitForGPU(const GfxDevice& gfxDevice, FrameSync& frameSync);
void MoveToNextFrame(const GfxDevice& gfxDevice, const Swapchain& swapchain, FrameSync& frameSync);

void DestroyFrameSyncResources(GfxDevice& gfxDevice, FrameSync& frameSync);