#pragma once
#include "Graphics/D3D12/Pipeline.h"
#include "Renderer/Core/RenderPass.h"
#include "Renderer/Core/Resources.h"

struct ComputeAABBData
{
	DirectX::XMMATRIX inverseProj;

	u32 clusterInputData[3];
	float zNear;

	u32 screenDimensions[2];
	float zFar;
	u32 clusterUAVIndex;
};

struct ComputeAABBPass : RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle ComputeAABBPipeline = g_invalidPipelineHandle;
	ResourceHandle ClusterResourceHandle = g_invalidResourceHandle;

	u32 clusterSizeXYZ[3]{ 16, 9, 24 };
};