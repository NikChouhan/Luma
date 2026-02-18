#pragma once
#include "Renderer/Core/RenderPass.h"

struct LightAssignCluster
{
	DirectX::XMMATRIX viewMatrix;

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
	auto Init() -> void override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;

private:
	PipelineHandle LightAssignClusterPipeline = g_invalidPipelineHandle;

	u32 clusterSizeXYZ[3] { 16, 9, 24};

	LightAssignCluster pushConstants{};

};
