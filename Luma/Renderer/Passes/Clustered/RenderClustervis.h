#pragma once
#include "Graphics/D3D12/Pipeline.h"
#include "Renderer/Core/RenderPass.h"
#include "Renderer/Core/Resources.h"

struct ClusterRender
{
	DirectX::XMMATRIX viewProj;

	u32 clusterGeometryStructuredBufferSRVIndex;
	SM::Vector3 padding;
};

struct RenderClusterVis : RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle ClusterRenderPipeline = g_invalidPipelineHandle;
	ClusterRender pushConstants{};
};
