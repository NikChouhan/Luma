#pragma once


struct Scene;
struct RenderContext;
struct PipelineCache;
struct ResourceManager;

struct RenderPass
{
	virtual ~RenderPass() = default;

	virtual void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) = 0;
	virtual void Execute(RenderContext& ctx, const Scene& scene) = 0;
};
