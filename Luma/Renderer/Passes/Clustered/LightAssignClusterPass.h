#pragma once
#include "Graphics/D3D12/Pipeline.h"
#include "Renderer/Core/RenderPass.h"
#include "Renderer/Core/Resources.h"
u32 constexpr max_lights = 256;
u32 constexpr no_of_clusters = 16 * 9 * 24;

u32 constexpr maxLightIndices = no_of_clusters * max_lights;


struct LightAssignCluster
{
	u32 clusterInputData[3];
	u32 clusterUAVIndex;

	u32 LightListTextureUAVIndex;
	u32 LightCounterUAVIndex;
	u32 LightIndexBufferUAVIndex;
	u32 lightCount;

	u32 globalLightStructuredBufferUAVIndex;
	u32 padding[3];
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

struct LightAssignClusterPass : public RenderPass
{
	auto Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) -> void override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle LightAssignClusterPipeline = g_invalidPipelineHandle;

	u32 clusterSizeXYZ[3] { 16, 9, 24};

	LightAssignCluster pushConstants{};

};
