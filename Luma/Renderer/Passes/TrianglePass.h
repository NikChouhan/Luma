#pragma once
#include "Graphics/RHI/RHITypes.h"
#include "Renderer/Core/RenderPass.h"

struct TrianglePass : public RenderPass
{
	void Init() override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;

private: 
	PipelineHandle pipelineHandle;
	BufferHandle vbHandle;
	BufferHandle ibHandle;
};