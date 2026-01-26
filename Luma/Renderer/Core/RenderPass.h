#pragma once

struct Scene;
struct RenderContext;
struct PipelineCache;
struct ResourceManager;

struct RenderPass
{
	virtual ~RenderPass() = default;

	virtual void Init() = 0;
	virtual void Execute(RenderContext& ctx, const Scene& scene) = 0;

	ResourceManager* resourceManager_;
	PipelineCache* pipelineCache_;
};
