#include "Renderer.h"

#include "Core/Resources.h"
#include "Graphics/FrameSync.h"
#include "Graphics/D3D12/CommandList.h"
#include "Graphics/D3D12/Swapchain.h"
#include "Passes/GeometryPass.h"

Renderer::Renderer(const GfxDevice& gfxDevice, 
	FrameSync& frameSync,
	Swapchain& swapchain,
	ResourceManager* resourceManager, 
	PipelineCache* pipelineCache)
	: gfxDevice_(gfxDevice), frameSync_(frameSync), swapchain_(swapchain),
      resourceManager_(resourceManager),
	  pipelineCache_(pipelineCache) {}

void Renderer::Init()
{
	commandList = CommandList(gfxDevice_).commandList_;

	for (const auto& pass : passes_)
	{
		pass->Init(resourceManager_, pipelineCache_);
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
	WaitForGPU(gfxDevice_, frameSync_);
	u32 frameIdx = frameSync_.frameIndex_;

	auto allocator = gfxDevice_.commandAllocators_[frameIdx];
	DX_ASSERT(allocator->Reset());
	DX_ASSERT(commandList->Reset(allocator.Get(), nullptr));

	// throughout the renderer life, I will be using a single bindless heap for SRVs/UAVs
	auto bindlessHeap = resourceManager_->GetBindlessHeap();
	ID3D12DescriptorHeap* heaps[] = { bindlessHeap.Get()};
	commandList->SetDescriptorHeaps(1, heaps);

	auto backBuffer = swapchain_.renderTargets_[frameIdx];
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer.Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(swapchain_.rtvHeap_->GetCPUDescriptorHandleForHeapStart(),
		frameSync_.frameIndex_, swapchain_.rtvDescriptorSize_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(swapchain_.dsvDepthHeap_->GetCPUDescriptorHandleForHeapStart());

	const float clearColor[] = { 0.4f, 0.2f, 0.7f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	return RenderContext{
		.cmdList_ = commandList.Get(),
		.frameIndex_ = frameIdx,
		.gfxDevice_ = gfxDevice_,
		.currentRtv = rtvHandle,
		.currentDsv = dsvHandle,
		.viewport = swapchain_.viewport_,
		.scissorRect = swapchain_.scissorRect_
	};
}

void Renderer::EndFrame(const RenderContext& ctx) const
{
	auto backBuffer = swapchain_.renderTargets_[ctx.frameIndex_];
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	ctx.cmdList_->ResourceBarrier(1, &barrier);
	DX_ASSERT(ctx.cmdList_->Close());

	ID3D12CommandList* ppCommandLists[] = { ctx.cmdList_};
	gfxDevice_.commandQueue_->ExecuteCommandLists(1, ppCommandLists);

	DX_ASSERT(swapchain_.swapchain_->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	MoveToNextFrame(gfxDevice_, swapchain_, frameSync_);
}
