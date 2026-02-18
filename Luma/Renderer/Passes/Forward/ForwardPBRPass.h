#pragma once
#include "External/SimpleMath/SimpleMath.h"
#include "Graphics/RHI/RHITypes.h"
#include "Renderer/Core/RenderPass.h"

namespace SM = DirectX::SimpleMath;

struct ForwardPBRPassRootConstants
{
    DirectX::XMMATRIX worldViewProj;

    u32 albedoIndex;
    u32 normalIndex;
    u32 metallicRoughnessIndex;
    u32 emissiveIndex;
};


struct ForwardPBRPass : RenderPass
{
	void Init() override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;

private:
	PipelineHandle rasterPipelineHandle = g_invalidPipelineHandle;

    ForwardPBRPassRootConstants pushConstants{};

};

