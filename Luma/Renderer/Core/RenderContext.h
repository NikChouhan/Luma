#pragma once
#include "Resources.h"
#include "Graphics/GfxDevice.h"

struct RenderContext
{
    ID3D12GraphicsCommandList* cmdList_;
    u32 frameIndex_;

    const GfxDevice& gfxDevice_;
};
