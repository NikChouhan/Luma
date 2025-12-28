
#pragma once
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderPass.h"

struct RasterPass : public RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle RasterPipelineHandle_ = g_invalidPipelineHandle;
};
