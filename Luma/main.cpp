#include <pch.h>

#include <windowsx.h>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Graphics/GfxDevice.h"
#include "Graphics/D3D12/Swapchain.h"
#include "Graphics/D3D12/Pipeline.h"
#include "Graphics/FrameSync.h"
#include "Graphics/D3D12/Buffer.h"
#include "Graphics/D3D12/Texture.h"
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
#include "Renderer/Passes/Clustered/ClusteredForward.h"


extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
	// Add passes
	renderer.AddPass<GeometryPass>();
	renderer.AddPass<ClusteredForward>();

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