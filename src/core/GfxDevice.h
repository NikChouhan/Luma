#pragma once

namespace D3D12MA
{
	class Allocator;
}

struct GfxDevice
{
	CD3DX12_VIEWPORT _viewport;
	CD3DX12_RECT _scissorRect;
	HWND _hwnd;
	ComPtr<IDXGIFactory2> _factory;
	ComPtr<ID3D12Device14> _device;
	ComPtr<ID3D12CommandAllocator> _commandAllocators[frameCount];
	ComPtr<ID3D12CommandQueue> _commandQueue;
	D3D12MA::Allocator* _allocator;
};

struct GfxDeviceDesc
{
	u32 _width;
	u32 _height;
	f32 _aspectRatio;
	HWND _hwnd;
};

GfxDevice CreateDevice(GfxDeviceDesc desc);
void DestroyDevice(GfxDevice& gfxDevice);
