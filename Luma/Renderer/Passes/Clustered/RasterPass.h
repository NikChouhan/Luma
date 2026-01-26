#pragma once
#include "Renderer/Core/RenderPass.h"

struct RasterPassRootConstants
{
    DirectX::XMMATRIX worldViewProj;
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

    float farZ;
    float nearZ;
    SM::Vector2 padding;

    SM::Vector3 cameraPosition;
    float padding2;
};


struct RasterPass : RenderPass
{
	void Init() override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle RasterPipelineHandle_ = g_invalidPipelineHandle;

    RasterPassRootConstants pushConstants{};

};

