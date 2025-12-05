#pragma once

#include "Common.h"
#include <functional>

struct Pipeline;
struct FrameSync;
struct ImmediateContext;

namespace D3D12MA
{
	class Allocator;
}

struct GfxDevice
{
	ComPtr<ID3D12Device14> device;
	ComPtr<ID3D12CommandAllocator> commandAllocators[frameCount];
	ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12MA::Allocator* allocator;
};

struct GfxDeviceDesc
{
};

GfxDevice CreateDevice(GfxDeviceDesc desc);
void DestroyDevice(GfxDevice& gfxDevice);

ComPtr<ID3D12GraphicsCommandList10> CreateCommandList(const GfxDevice& gfxDevice);
void ImmediateSubmit(const GfxDevice& gfxDevice, ImmediateContext* immediateCtx, LAMBDA() callback);
