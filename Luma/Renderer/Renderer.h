#pragma once
#include "Core/RenderContext.h"
#include "Graphics/GfxDevice.h"
#include "Renderer/Core/RenderPass.h"

struct Swapchain;
struct Scene;
struct PipelineCache;
struct ResourceManager;

struct Renderer
{
	Renderer(const GfxDevice& gfxDevice,
		FrameSync& frameSync,
		Swapchain& swapchain,
		ResourceManager* resourceManager,
		PipelineCache* pipelineCache);
	~Renderer() = default;
	template<typename T>
	void AddPass();
	void Init();
	void RenderFrame(const Scene& scene) const;

private:
	const GfxDevice& gfxDevice_;
	FrameSync& frameSync_;
	Swapchain& swapchain_;
	ResourceManager* resourceManager_ = nullptr;
	PipelineCache* pipelineCache_ = nullptr;

	std::vector<std::unique_ptr<RenderPass>> passes_;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	RenderContext BeginFrame() const;
	void EndFrame(const RenderContext& ctx) const;
};

template <typename T>
void Renderer::AddPass()
{
	passes_.push_back(std::make_unique<T>());
}
