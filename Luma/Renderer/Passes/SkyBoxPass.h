#pragma once
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
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle SkyBoxPipelineHandle = g_invalidPipelineHandle;

	BufferHandle skyboxVertexBufferHandle = g_invalidBufferHandle;
	BufferHandle skyboxIndexBufferHandle = g_invalidBufferHandle;
	BufferHandle skyboxCubemapHandle = g_invalidBufferHandle;
	// texture for skybox
};