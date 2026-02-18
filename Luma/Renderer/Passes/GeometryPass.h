#pragma once
#include <Graphics/RHI/RHITypes.h>

#include "Renderer/Core/RenderPass.h"

struct DepthPassRootConstants
{
	DirectX::XMMATRIX worldViewProj;
	DirectX::XMMATRIX worldMatrix;
};

struct GeometryPass : public RenderPass
{
	void Init() override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;

private:
	PipelineHandle pipelineHandle = g_invalidPipelineHandle;
};