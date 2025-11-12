#include "scene.h"

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
	scene._commandList = CreateCommandList(gfxDevice);
	DX_ASSERT(scene._commandList->Close());

	if (sceneDesc._sceneType == SceneType::SPONZA) scene._modelPath = "../../../../assets/models/sponza2/sponza2.gltf";
	else if (sceneDesc._sceneType == SceneType::SPONZA) scene._modelPath = "../../../../assets/models/bistro3/scene.gltf";

	scene.model = LoadModel(gfxDevice, frameSync, swapchain, scene._commandList.Get(),
		{
		._path = scene._modelPath });
	scene.dxcRes = ShaderCompiler();

	ShaderDesc backdropDesc = {
		._shaderPath = L"../../../../shaders/shaders/space_backdrop.hlsl",
		._pEntryPoint = L"CSMain",
		._pTarget = L"cs_6_7",
		._type = Type::COMPUTE
	};
	Shader backdropCS = CreateShader(gfxDevice, &scene.resources, scene.dxcRes, backdropDesc);

	ShaderDesc depthPPDesc = {
		._shaderPath = L"../../../../shaders/shaders/depth_pass.hlsl",
		._pEntryPoint = L"DepthVS",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX };
	Shader depthPrePassVS = CreateShader(gfxDevice, &scene.resources, scene.dxcRes, depthPPDesc);

	ShaderDesc inlineRTVertexDesc = {
		._shaderPath = L"../../../../shaders/shaders/model.hlsl",
		._pEntryPoint = L"VSMain",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX };
	Shader inlineRayTracingVertexShader = CreateShader(gfxDevice, &scene.resources, scene.dxcRes, inlineRTVertexDesc);

	ShaderDesc inlineRTPixelDesc = {
		._shaderPath = L"../../../../shaders/shaders/model.hlsl",
		._pEntryPoint = L"PSMain",
		._pTarget = L"ps_6_7",
		._type = Type::PIXEL };
	Shader inlineRayTracingPixelShader = CreateShader(gfxDevice, &scene.resources, scene.dxcRes, inlineRTPixelDesc);

	// compute pass for shader effects
	PipelineDesc backfropComputePDesc = {
		._pipelineType = PipelineType::COMPUTE,
		._shaderIndex = {0},
		._enableDepthTest = FALSE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE };
	Pipeline backdropComputePipeline = CreatePipeline(gfxDevice, swapchain, &scene.resources, backfropComputePDesc);

	// depth pipeline
	PipelineDesc depthPassDesc = {
		._pipelineType = PipelineType::GRAPHICS,
		._shaderIndex = {1} ,
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = TRUE };
	Pipeline depthPrePass = CreatePipeline(gfxDevice, swapchain, &scene.resources, depthPassDesc);

	// inline ray tracing pass
	PipelineDesc rtPipelineDesc = {
		._pipelineType = PipelineType::GRAPHICS,
		._shaderIndex = {2,3},
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE };
	Pipeline rTPipeline= CreatePipeline(gfxDevice, swapchain, &scene.resources, rtPipelineDesc);

	// imgui parts
	scene.inspector.CreateInspector(gfxDevice, swapchain, frameSync);
	scene.inspector.io->IniFilename = "imgui.ini";

	WaitForGPU(gfxDevice, frameSync);

	return scene;
}

void Scene::HotReload(GfxDevice& gfxDevice, Swapchain& swapchain, DXCRes& dxcRes, Resources& resources)
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

	std::wstring watchDirectory = L"../../../../shaders";

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
			HotReload(gfxDevice, swapchain, dxcRes, resources);
		}
		ImGui::End();
	}
	SubmitPasses(_commandList, gfxDevice, swapchain, frameSync, inspector, 
		camera, resources.pipelines[0],
		resources.pipelines[1], resources.pipelines[2], model);

	// present the Frame
	DX_ASSERT(swapchain._swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	MoveToNextFrame(gfxDevice, swapchain, frameSync);
}