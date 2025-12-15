#pragma once
#include "Renderer/Core/RenderPass.h"

struct GeometryPass : public RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;
};