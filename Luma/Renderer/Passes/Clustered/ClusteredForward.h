#pragma once
#include "Graphics/D3D12/Pipeline.h"
#include "Renderer/Core/RenderPass.h"
#include "Renderer/Core/Resources.h"
u32 constexpr max_lights = 4096;
u32 constexpr no_of_clusters = 16 * 9 * 24;

u32 constexpr maxLightIndices = no_of_clusters * max_lights;

struct ComputeAABBData
{
	DirectX::XMMATRIX inverseProj;

	u32 clusterInputData[3];
	float zNear;

	u32 screenDimensions[2];
	float zFar;
	u32 clusterUAVIndex;
};

struct LightAssignCluster
{
	u32 clusterInputData[3];
	u32 clusterUAVIndex;

	u32 LightListTextureUAVIndex;
	u32 LightCounterUAVIndex;
	u32 LightIndexBufferUAVIndex;
	u32 lightCount;
};

struct LightListCounterBuffer
{
	
};

struct Cluster
{
	SM::Vector4 minPoint;
	SM::Vector4 maxPoint;
};

struct LightIndexList
{
	
};

struct ClusteredForward : public RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle ComputeAABBPipeline = g_invalidPipelineHandle;
	ResourceHandle ClusterResourceHandle = g_invalidResourceHandle;

	PipelineHandle LightAssignClusterPipeline = g_invalidPipelineHandle;
	ResourceHandle LightIndexListTextureHandle = g_invalidResourceHandle;
	ResourceHandle LightListCounterBufferHandle = g_invalidResourceHandle;
	ResourceHandle LightIndicesBufferHandle = g_invalidResourceHandle;

	u32 clusterSizeXYZ[3] { 16, 9, 24};
};
