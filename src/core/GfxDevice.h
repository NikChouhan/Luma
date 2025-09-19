#pragma once

#include "Common.h"
#include <functional>

struct FrameSync;

namespace D3D12MA
{
	class Allocator;
}

struct GfxDevice
{
	ComPtr<IDXGIFactory2> _factory;
	ComPtr<ID3D12Device14> _device;
	ComPtr<ID3D12CommandAllocator> _commandAllocators[frameCount];
	ComPtr<ID3D12CommandQueue> _commandQueue;
	D3D12MA::Allocator* _allocator;
};

struct GfxDeviceDesc
{
};

GfxDevice CreateDevice(GfxDeviceDesc desc);
void DestroyDevice(GfxDevice& gfxDevice);

ComPtr<ID3D12GraphicsCommandList1> CreateCommandList(GfxDevice& gfxDevice);
void ImmediateSubmit(GfxDevice& gfxDevice, FrameSync& framesync, LAMBDA(ComPtr<ID3D12GraphicsCommandList1>) callback);
