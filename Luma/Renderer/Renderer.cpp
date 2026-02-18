#include "Renderer.h"

#include "Graphics/RHI/RHI.h"
#include "Passes/GeometryPass.h"

Renderer::Renderer()
{
	cl = RHI::CreateCommandList(false);
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
	RenderCtx ctx = BeginFrame();

	for (auto& pass : passes_)
	{
		pass->Execute(ctx, scene);
	}
	EndFrame(ctx);
}

RenderCtx Renderer::BeginFrame() const
{
	RHI::BeginFrame();
	cl->Begin();
	return RenderCtx{ cl };
}

void Renderer::EndFrame(const RenderCtx& ctx) const
{
	// rhi impl
	cl->EndRendering();
	cl->End();
	cl->Submit();
	RHI::EndFrame();
}
