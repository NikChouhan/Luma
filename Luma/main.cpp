#include <pch.h>

#include <windowsx.h>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Graphics/GfxDevice.h"
#include "Graphics/D3D12/Swapchain.h"
#include "Graphics/FrameSync.h"
#include "Graphics/D3D12/Buffer.h"
#include "Core/Log.h"

#include "Graphics/Globals.h"

#include "Core/Camera.h"
#include "Renderer/Model.h"
#include "Renderer/Core/Inspector.h"

#include "scene.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Passes/GeometryPass.h"
#include "Renderer/Passes/Clustered/RasterPass.h"
#include "Renderer/Passes/SkyBoxPass.h"
#include "Renderer/Passes/Clustered/ComputeAABBPass.h"
#include "Renderer/Passes/Clustered/LightAssignClusterPass.h"
#include "Renderer/Passes/Clustered/MarkActiveClusters.h"



extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static u32 CreateDepthSRV(const GfxDevice& gfxDevice, ID3D12Resource* depthResource, ID3D12DescriptorHeap* bindlessHeap)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	u32 index = GlobalStorage::bindlessHeapIndex.nextIndex++;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(bindlessHeap->GetCPUDescriptorHandleForHeapStart());
	u32 descriptorSize = gfxDevice.device_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.Offset(index, descriptorSize);

	gfxDevice.device_->CreateShaderResourceView(depthResource, &srvDesc, cpuHandle);

	return index;
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int cmdShow
)
{
	Log::Init();

	GfxDevice gfxDevice = CreateDevice({.something = true});
	FrameSync frameSync = CreateFrameSyncResources(gfxDevice);
	Window window(gfxDevice, frameSync,
		{
		.width = 1920,
		.height = 1080,
		.title = L"Luma" });
	Timer timer{};
	Swapchain swapchain = CreateSwapChain(gfxDevice, frameSync,
		{
			.height_ = u16(GlobalStorage::g_LumaConstants.height),
			.width_ = u16(GlobalStorage::g_LumaConstants.width),
			.vsyncEnable_ = true,
			.hwnd_ = window.GetHandle()
		});
	//SetupLightSettingsHandler();

	ResourceManager resourceManager(gfxDevice, frameSync);
	PipelineCache pipelineCache(gfxDevice);

	Scene scene(gfxDevice, resourceManager);
	scene.Load();

	Renderer renderer(gfxDevice, frameSync, swapchain, &resourceManager, &pipelineCache);
	// the worst way to do ts i know but i dont have time. fuck it we ball
	GlobalStorage::depthSRVIndex = CreateDepthSRV(gfxDevice, swapchain.depthStencil_.Get(), resourceManager.GetBindlessHeap().Get());
	// Add passes
	renderer.AddPass<SkyBoxPass>();
	renderer.AddPass<GeometryPass>();

	renderer.AddPass<ComputeAABBPass>();
	renderer.AddPass<MarkActiveClusters>();
	renderer.AddPass<LightAssignClusterPass>();

	renderer.AddPass<RasterPass>();

	renderer.Init();

	timer.Reset();

	while (window.PollEvents())
	{
		timer.Tick();
		float dt = timer.DeltaTime();

		scene.Update(dt);
		renderer.RenderFrame(scene);

		window.SetTitle(std::to_wstring(1./dt));
	}
	return 0;
}

// [INFO] [INFO] Camera pos-> x:17.133108, y: -1.0338303, z: -8.97141 -> min
// [INFO] Camera pos->x:-14.823427, y : 12.848415, z : 8.941228 ->  max
