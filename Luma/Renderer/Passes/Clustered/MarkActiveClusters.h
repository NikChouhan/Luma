#pragma once
#include "Renderer/Core/RenderPass.h"

struct MarkActiveClusters : RenderPass
{
	void Init() override;
	void Execute(RenderCtx& ctx, const Scene& scene) override;
};
