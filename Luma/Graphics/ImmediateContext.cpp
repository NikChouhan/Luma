#include "ImmediateContext.h"

ImmediateContext CreateImmediateContext(const GfxDevice& gfxDevice)
{
	ImmediateContext immediateContext{};

	DX_ASSERT(gfxDevice.device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&immediateContext.cmdAllocator)));

	DX_ASSERT(gfxDevice.device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		immediateContext.cmdAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&immediateContext.commandList)));
	DX_ASSERT(immediateContext.commandList->Close());

	DX_ASSERT(gfxDevice.device_->CreateFence(0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&immediateContext.fence)));
	immediateContext.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!immediateContext.fenceEvent)
	{
		printl(Log::LogLevel::Error, "Failed to create fence event, for Immediate context!");
		abort();
	}

	return immediateContext;
}