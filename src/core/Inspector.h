#pragma once

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include "GfxDevice.h"

struct DescriptorHeapAllocator;
struct Swapchain;

struct Inspector
{
	ComPtr<ID3D12DescriptorHeap> _imguiHeap;
	ImGuiIO* io;

	void CreateImguiHeap(const GfxDevice& gfxDevice);
	void CreateInspector(GfxDevice& gfxDevice, Swapchain& swapchain, FrameSync& frameSync);
};
