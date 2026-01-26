#pragma once
#include "Renderer/Core/RenderPass.h"

struct TrianglePass : public RenderPass
{
	void Init() override;
	void Execute(RenderContext& ctx, const Scene& scene) override;
};