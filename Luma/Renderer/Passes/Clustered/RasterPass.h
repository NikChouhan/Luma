#pragma once
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderPass.h"

struct RasterPassRootConstants
{
    DirectX::XMMATRIX worldViewProj;
    DirectX::XMMATRIX worldMatrix;
    DirectX::XMMATRIX inverseViewProj;

    u32 albedoIndex;
    u32 normalIndex;
    u32 metallicRoughnessIndex;
    u32 emissiveIndex;

    SM::Vector2 ScreenResolution;
    u32 clusterIndex;
    u32 depthSRVIndex;

    u32 lightListCounterBufferSRVIndex;
    u32 lightListTextureSRVIndex;
    u32 lightIndicesBufferSRVIndex;
    u32 globalLightStructuredBufferSRVIndex;
};


struct RasterPass : RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle RasterPipelineHandle_ = g_invalidPipelineHandle;

    RasterPassRootConstants pushConstants{};

};

