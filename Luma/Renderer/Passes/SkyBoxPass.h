#pragma once
#include "Renderer/Core/RenderPass.h"
#include "Graphics/D3D12/Pipeline.h"
#include "Renderer/Core/Resources.h"

struct SkyBoxConstants
{
	DirectX::XMMATRIX viewProj{};

	u32 cubemapSRVIndex{};
	u32 padding[3];
};

struct SkyBoxPass : public RenderPass
{
	void Init(ResourceManager* resourceManager, PipelineCache* pipelineCache) override;
	void Execute(RenderContext& ctx, const Scene& scene) override;

private:
	PipelineHandle SkyBoxPipelineHandle = g_invalidPipelineHandle;

	ResourceHandle skyboxVertexBufferHandle = g_invalidResourceHandle;
	ResourceHandle skyboxIndexBufferHandle = g_invalidResourceHandle;
	ResourceHandle skyboxCubemapHandle = g_invalidResourceHandle;
	// texture for skybox
};