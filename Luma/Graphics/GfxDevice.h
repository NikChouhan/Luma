#pragma once
#include <functional>
#include "Core/Common.h"

struct Pipeline;
struct FrameSync;
struct ImmediateContext;

namespace D3D12MA
{
	class Allocator;
}

struct GfxDevice
{
	ComPtr<ID3D12Device14> device_;
	ComPtr<ID3D12CommandAllocator> commandAllocators_[frameCount];
	ComPtr<ID3D12CommandQueue> commandQueue_;
	D3D12MA::Allocator* allocator_;

	ComPtr<IDXGIFactory2> factory_;
};

struct GfxDeviceDesc
{
};

GfxDevice CreateDevice(GfxDeviceDesc desc);
void DestroyDevice(GfxDevice& gfxDevice);

void ImmediateSubmit(const GfxDevice& gfxDevice, ImmediateContext* immediateCtx, LAMBDA() callback);
