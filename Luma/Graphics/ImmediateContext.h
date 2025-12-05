#pragma once
#include "Graphics/GfxDevice.h"

struct ImmediateContext
{
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList1> commandList;
	ComPtr<ID3D12Fence> fence;
	u64 fenceValue = 0;
	HANDLE fenceEvent = nullptr;
};

ImmediateContext CreateImmediateContext(const GfxDevice& gfxDevice);