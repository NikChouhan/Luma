#include "scene.h"

#include <fstream>

#include "Graphics/GfxDevice.h"
#include "Graphics/D3D12/Swapchain.h"
#include "Graphics/D3D12/Pipeline.h"
#include "Graphics/FrameSync.h"
#include "Graphics/D3D12/Buffer.h"
#include "Graphics/D3D12/Shader.h"
#include "Renderer/Inspector.h"
#include "Renderer/Model.h"
#include "Renderer/Resources.h"

Scene CreateScene(GfxDevice& gfxDevice, 
                  FrameSync& frameSync, 
                  Swapchain& swapchain, 
                  Camera& camera, 
                  SceneDesc sceneDesc)
{
	Scene scene{};

	sceneDesc.model = &scene.model;

	scene.resourceManager = new ResourceManager(gfxDevice, frameSync);

	static u32 pipelineCount = 0;
	static u32 shaderCount = 0;
	scene.commandList = CreateCommandList(gfxDevice);
	DX_ASSERT(scene.commandList->Close());

	scene.dxcRes = ShaderCompiler();



	// imgui parts
	scene.inspector.CreateInspector(gfxDevice, swapchain, frameSync);
	scene.inspector.io->IniFilename = "imgui.ini";

	WaitForGPU(gfxDevice, frameSync);

	return scene;
}


void Scene::Render(GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, Camera& camera)
{
	WaitForGPU(gfxDevice, frameSync);
	DX_ASSERT(gfxDevice.commandAllocators[frameSync._frameIndex]->Reset());
	DX_ASSERT(commandList->Reset(gfxDevice.commandAllocators[frameSync._frameIndex].Get(), nullptr));

	// imgui frame init
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// imgui frame stats
	{
		ImGui::Begin("Frame stats!");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / inspector.io->Framerate, inspector.io->Framerate);
		ImGui::End();
	}
	// hot reload
	{
		ImGui::Begin("Hot reload");
		if (ImGui::Button("Compile!"))
		{
			// hot reload
		}
		ImGui::End();
	}

	// submit passes

	// execute passes
	{
		CD3DX12_RESOURCE_BARRIER rBarriers;
		// transition the render target to present format
		rBarriers = { CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT) };
		commandList->ResourceBarrier(1, &rBarriers);

		DX_ASSERT(commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
		gfxDevice.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	// present the Frame
	DX_ASSERT(swapchain._swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	MoveToNextFrame(gfxDevice, swapchain, frameSync);
}