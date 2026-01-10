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

	u32 clusterGeometryStructuredBufferUAVndex;
	SM::Vector3 padding;
};

struct ClusterGeometryData
{
	SM::Vector4 vertices[8];
	struct {
		u32 indices[2];
		u32 padding[2];
	} triangles[18];
};

struct ClusterRender
{
	DirectX::XMMATRIX viewProj;

	u32 clusterGeometryStructuredBufferSRVIndex;
	SM::Vector3 padding;
};


struct ComputeAABBPass : RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle ComputeAABBPipeline = g_invalidPipelineHandle;
	u32 clusterSizeXYZ[3]{ 16, 9, 24 };
	ComputeAABBData pushConstants{};

	PipelineHandle ClusterRenderPipeline = g_invalidPipelineHandle;
	ClusterRender pushConstants1{};
};