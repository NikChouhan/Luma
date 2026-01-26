#include "Renderer.h"

#include "Graphics/RHI/RHI.h"
#include "Passes/GeometryPass.h"

Renderer::Renderer()
{
	cl = RHI::CreateCommandList();
}

void Renderer::Init()
{
	for (const auto& pass : passes_)
	{
		pass->Init();
	}
}

void Renderer::RenderFrame(const Scene& scene) const
{
	RenderContext ctx = BeginFrame();

	for (auto& pass : passes_)
	{
		pass->Execute(ctx, scene);
	}
	EndFrame(ctx);
}

RenderContext Renderer::BeginFrame() const
{
	RHI::BeginFrame();
	cl->Begin();
	return RenderContext{ cl };
}

void Renderer::EndFrame(const RenderContext& ctx) const
{
	// rhi impl
	cl->EndRendering();
	cl->End();
	cl->Submit();
	RHI::EndFrame();
}
