#pragma once
#include "Renderer/Core/RenderPass.h"

struct ClusterRender
{
	DirectX::XMMATRIX viewProj;

	u32 clusterGeometryStructuredBufferSRVIndex;
	SM::Vector3 padding;
};

struct RenderClusterVis : RenderPass
{
	void Init() override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;

private:
	PipelineHandle ClusterRenderPipeline = g_invalidPipelineHandle;
	ClusterRender pushConstants{};
};
