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

    u32 lightListCounterBufferUAVIndex;
    u32 lightListTextureUAVIndex;
    u32 lightIndicesBufferUAVIndex;
    u32 globalLightStructuredBufferUAVIndex;
};


struct RasterPass : RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle RasterPipelineHandle_ = g_invalidPipelineHandle;

    RasterPassRootConstants pushConstants{};

};

