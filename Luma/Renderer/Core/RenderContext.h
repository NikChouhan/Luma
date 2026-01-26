#pragma once
#include "Graphics/RHI/RHI.h"

struct RenderContext
{
    RHI::CommandList* cl;
    std::vector<TextureHandle> colorTargets;
    TextureHandle depthTarget;

    RHIViewPort viewport;
    RHIScissor scissor;

    // TODO: perform binding here
    //void BindRenderTargets() const
    //{
    //    cl->BeginRendering(colorTargets, depthTarget);
    //}
};
