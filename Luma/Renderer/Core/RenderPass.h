#pragma once

struct Scene;
struct RenderCtx;
struct PipelineCache;
struct ResourceManager;

struct RenderPass
{
	virtual ~RenderPass() = default;

	virtual void Init() = 0;
	virtual void Execute(RenderCtx& ctx, const Scene& scene) = 0;

	ResourceManager* resourceManager_;
	PipelineCache* pipelineCache_;
};
