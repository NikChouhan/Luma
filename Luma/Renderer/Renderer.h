#pragma once
#include "Core/RenderContext.h"
#include "Graphics/GfxDevice.h"

struct Swapchain;
struct Scene;
struct RenderPass;
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
	void Init();
	void RenderFrame(const Scene& scene);

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
