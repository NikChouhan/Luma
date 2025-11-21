#include "scene.h"

#include <thread>

#include "GfxDevice.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "FrameSync.h"
#include "Buffer.h"
#include "Inspector.h"
#include "Model.h"
#include "Resources.h"
#include "Texture.h"
#include "Watcher.h"


Scene CreateScene(GfxDevice& gfxDevice, 
                  FrameSync& frameSync, 
                  Swapchain& swapchain, 
                  Camera& camera, 
                  SceneDesc sceneDesc)
{
	Scene scene{};
	static u32 pipelineCount = 0;
	static u32 shaderCount = 0;
	scene._commandList = CreateCommandList(gfxDevice);
	DX_ASSERT(scene._commandList->Close());

	if (sceneDesc._sceneType == SceneType::SPONZA) scene._modelPath = "../../../../assets/models/sponza2/sponza2.gltf";
	else if (sceneDesc._sceneType == SceneType::SPONZA) scene._modelPath = "../../../../assets/models/bistro3/scene.gltf";

	scene.model = LoadModel(gfxDevice, frameSync, swapchain, scene._commandList.Get(),
		{
		._path = scene._modelPath });
	scene.dxcRes = ShaderCompiler();

	// shaders
	{
		ShaderDesc backdropDesc = {
		._shaderPath = L"../../../../shaders/space_backdrop.hlsl",
		._pEntryPoint = L"CSMain",
		._pTarget = L"cs_6_7",
		._type = Type::COMPUTE
		};
		Shader backdropCS = CreateShader(gfxDevice, &scene._resources, scene.dxcRes, backdropDesc);
		GlobalStorage::shaderIndex.bgComputeShader = shaderCount;

		ShaderDesc depthPPDesc = {
			._shaderPath = L"../../../../shaders/depth_pass.hlsl",
			._pEntryPoint = L"DepthVS",
			._pTarget = L"vs_6_7",
			._type = Type::VERTEX };
		Shader depthPrePassVS = CreateShader(gfxDevice, &scene._resources, scene.dxcRes, depthPPDesc);
		GlobalStorage::shaderIndex.depthPPShader = ++shaderCount;

		ShaderDesc inlineRTVertexDesc = {
			._shaderPath = L"../../../../shaders/model.hlsl",
			._pEntryPoint = L"VSMain",
			._pTarget = L"vs_6_7",
			._type = Type::VERTEX };
		Shader inlineRayTracingVertexShader = CreateShader(gfxDevice, &scene._resources, scene.dxcRes, inlineRTVertexDesc);
		GlobalStorage::shaderIndex.renderPassVSShader = ++shaderCount;

		ShaderDesc inlineRTPixelDesc = {
			._shaderPath = L"../../../../shaders/model.hlsl",
			._pEntryPoint = L"PSMain",
			._pTarget = L"ps_6_7",
			._type = Type::PIXEL };
		Shader inlineRayTracingPixelShader = CreateShader(gfxDevice, &scene._resources, scene.dxcRes, inlineRTPixelDesc);
		GlobalStorage::shaderIndex.renderPassPSShader = ++shaderCount;
	}
	// pipelines
	{
		// compute pass for shader effects
		PipelineDesc backfropComputePDesc = {
			._pipelineType = PipelineType::COMPUTE,
			._rootSignType = RootSignDesc::RSType::SHADER_EFFECTS,
			._shaderIndex = {GlobalStorage::shaderIndex.bgComputeShader},
			._enableDepthTest = FALSE,
			._enableStencilTest = FALSE,
			._isDepthPrePass = FALSE };
		Pipeline backdropComputePipeline = CreatePipeline(gfxDevice, swapchain, &scene._resources, backfropComputePDesc);
		GlobalStorage::pipelineIndex.BGComputePass = pipelineCount;

		// depth pipeline
		PipelineDesc depthPassDesc = {
			._pipelineType = PipelineType::GRAPHICS,
			._rootSignType = RootSignDesc::RSType::DEPTHPP,
			._shaderIndex = {GlobalStorage::shaderIndex.depthPPShader} ,
			._enableDepthTest = TRUE,
			._enableStencilTest = FALSE,
			._isDepthPrePass = TRUE };
		Pipeline depthPrePass = CreatePipeline(gfxDevice, swapchain, &scene._resources, depthPassDesc);
		GlobalStorage::pipelineIndex.DepthPrePass = ++pipelineCount;

		scene._rtaoPass = InitRTAOResources(gfxDevice, frameSync, swapchain, scene.dxcRes, &scene._resources);
		GlobalStorage::pipelineIndex.RTAOPass = ++pipelineCount;

		// inline ray tracing pass
		PipelineDesc rtPipelineDesc = {
			._pipelineType = PipelineType::GRAPHICS,
			._rootSignType = RootSignDesc::RSType::RENDER,
			._shaderIndex = {GlobalStorage::shaderIndex.renderPassVSShader, GlobalStorage::shaderIndex.renderPassPSShader},
			._enableDepthTest = TRUE,
			._enableStencilTest = FALSE,
			._isDepthPrePass = FALSE };
		Pipeline rtPipeline = CreatePipeline(gfxDevice, swapchain, &scene._resources, rtPipelineDesc);
		GlobalStorage::pipelineIndex.RenderPass = ++pipelineCount;
	}

	// imgui parts
	scene.inspector.CreateInspector(gfxDevice, swapchain, frameSync);
	scene.inspector.io->IniFilename = "imgui.ini";

	WaitForGPU(gfxDevice, frameSync);

	return scene;
}

void Scene::HotReload(const GfxDevice& gfxDevice, const Swapchain& swapchain, DXCRes& dxcRes, Resources& resources)
{
	for (auto& shader : resources.shaders)
		shader.Release();
	for (auto& pipeline : resources.pipelines)
		pipeline.Release();

	for (int i = 0; i < resources.shaders.size(); i++)
	{
		CompileShaderInternal(gfxDevice, dxcRes, resources.shaders[i], resources.shaderParams[i]);
	}
	for (int i = 0; i < resources.pipelines.size(); i++)
	{
		CompilePipelineInternal(gfxDevice, swapchain, resources.pipelines[i], &resources, resources.pipelineParams[i]);
	}
}

void Scene::Render(GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, Camera& camera)
{
	WaitForGPU(gfxDevice, frameSync);
	DX_ASSERT(gfxDevice._commandAllocators[frameSync._frameIndex]->Reset());
	DX_ASSERT(_commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(), nullptr));

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
			HotReload(gfxDevice, swapchain, dxcRes, _resources);
		}
		ImGui::End();
	}
	SubmitPasses(_commandList, gfxDevice, swapchain, frameSync, inspector, 
		camera, _resources.pipelines, model);

	// execute passes
	{
		CD3DX12_RESOURCE_BARRIER rBarriers;
		// transition the render target to present format
		rBarriers = { CD3DX12_RESOURCE_BARRIER::Transition(swapchain._renderTargets[frameSync._frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT) };
		_commandList->ResourceBarrier(1, &rBarriers);

		DX_ASSERT(_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
		gfxDevice._commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	// present the Frame
	DX_ASSERT(swapchain._swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	MoveToNextFrame(gfxDevice, swapchain, frameSync);
}