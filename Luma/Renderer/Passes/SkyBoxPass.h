#pragma once
#include "Graphics/RHI/RHITypes.h"
#include "Renderer/Core/RenderPass.h"

struct SkyBoxConstants
{
	DirectX::XMMATRIX viewProj{};

	u32 cubemapSRVIndex{};
	u32 padding[3];
};

struct SkyBoxPass : public RenderPass
{
	void Init() override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;

private:
	PipelineHandle skyBoxPipelineHandle = g_invalidPipelineHandle;

	BufferHandle skyboxVertexBufferHandle = g_invalidBufferHandle;
	BufferHandle skyboxIndexBufferHandle = g_invalidBufferHandle;
	TextureHandle skyboxCubemapHandle = g_invalidTextureHandle;
	// texture for skybox
};