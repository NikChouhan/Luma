#pragma once
#include "Renderer/Core/RenderPass.h"

struct MarkActiveClusters : RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;
};
