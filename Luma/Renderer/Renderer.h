#pragma once
#include "Core/RenderContext.h"
#include "Renderer/Core/RenderPass.h"

struct Swapchain;
struct Scene;
struct PipelineCache;
struct ResourceManager;

struct Renderer
{
	Renderer();
	~Renderer() = default;
	template<typename T>
	void AddPass();
	void Init();
	void RenderFrame(const Scene& scene) const;

private:
	std::vector<std::unique_ptr<RenderPass>> passes_;
	RHI::CommandList* cl;
	RenderContext BeginFrame() const;
	void EndFrame(const RenderContext& ctx) const;
};

template <typename T>
void Renderer::AddPass()
{
	passes_.push_back(std::make_unique<T>());
}
