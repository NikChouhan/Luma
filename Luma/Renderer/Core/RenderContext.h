#pragma once
#include "Resources.h"
#include "Graphics/GfxDevice.h"

struct RenderContext
{
    ID3D12GraphicsCommandList* cmdList_;
    u32 frameIndex_;

    const GfxDevice& gfxDevice_;

    D3D12_CPU_DESCRIPTOR_HANDLE currentRtv;     // Backbuffer RTV handle
    D3D12_CPU_DESCRIPTOR_HANDLE currentDsv;     // Depth Stencil handle
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissorRect;
};
