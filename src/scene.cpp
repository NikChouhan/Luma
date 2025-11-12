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

	wchar_t backdropCSPath[] = L"../../../../shaders/shaders/space_backdrop.hlsl";
	ShaderDesc backdropDesc = {
		.resourcePtr = &scene.resources,
		._shaderPath = backdropCSPath,
		._pEntryPoint = L"CSMain",
		._pTarget = L"cs_6_7",
		._type = Type::COMPUTE
	};
	Shader backdropCS = CreateShader(gfxDevice, scene.dxcRes, backdropDesc);

	wchar_t depthPPVertexShaderPath[] = L"../../../../shaders/shaders/depth_pass.hlsl";
	ShaderDesc depthPPDesc = {
		.resourcePtr = &scene.resources,
		._shaderPath = depthPPVertexShaderPath,
		._pEntryPoint = L"DepthVS",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX };
	Shader depthPrePassVS = CreateShader(gfxDevice, scene.dxcRes, depthPPDesc);

	wchar_t inlineRayTracingVertexShaderPath[] = L"../../../../shaders/shaders/model.hlsl";
	ShaderDesc inlineRTVertexDesc = {
		.resourcePtr = &scene.resources,
		._shaderPath = inlineRayTracingVertexShaderPath,
		._pEntryPoint = L"VSMain",
		._pTarget = L"vs_6_7",
		._type = Type::VERTEX };
	Shader inlineRayTracingVertexShader = CreateShader(gfxDevice, scene.dxcRes, inlineRTVertexDesc);

	wchar_t inlineRayTracingPixelShaderPath[] = L"../../../../shaders/shaders/model.hlsl";
	ShaderDesc inlineRTPixelDesc = {
		.resourcePtr = &scene.resources,
		._shaderPath = inlineRayTracingPixelShaderPath,
		._pEntryPoint = L"PSMain",
		._pTarget = L"ps_6_7",
		._type = Type::PIXEL };
	Shader inlineRayTracingPixelShader = CreateShader(gfxDevice, scene.dxcRes, inlineRTPixelDesc);

	// compute pass for shader effects
	PipelineDesc backfropComputePDesc = {
		.resourcePtr = &scene.resources,
		._pipelineType = PipelineType::COMPUTE,
		._shaders = {backdropCS},
		._enableDepthTest = FALSE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE };
	Pipeline backdropComputePipeline = CreatePipeline(gfxDevice, swapchain, backfropComputePDesc);

	// depth pipeline
	PipelineDesc depthPassDesc = {
		.resourcePtr = &scene.resources,
		._pipelineType = PipelineType::GRAPHICS,
		._shaders = {depthPrePassVS} ,
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = TRUE };
	Pipeline depthPrePass = CreatePipeline(gfxDevice, swapchain, depthPassDesc);

	// inline ray tracing pass
	PipelineDesc rtPipelineDesc = {
		.resourcePtr = &scene.resources,
		._pipelineType = PipelineType::GRAPHICS,
		._shaders = {inlineRayTracingVertexShader, inlineRayTracingPixelShader},
		._enableDepthTest = TRUE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE };
	Pipeline rTPipeline= CreatePipeline(gfxDevice, swapchain, rtPipelineDesc);

	// imgui parts
	scene.inspector.CreateInspector(gfxDevice, swapchain, frameSync);
	scene.inspector.io->IniFilename = "imgui.ini";

	WaitForGPU(gfxDevice, frameSync);

	return scene;
}

void Scene::Render(GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, Camera& camera)
{
	WaitForGPU(gfxDevice, frameSync);
	DX_ASSERT(gfxDevice._commandAllocators[frameSync._frameIndex]->Reset());
	DX_ASSERT(_commandList->Reset(gfxDevice._commandAllocators[frameSync._frameIndex].Get(), nullptr));

	std::wstring watchDirectory = L"../../../../shaders";

	/*if (bool isChange = WatchDirectory(watchDirectory))
	{
		printl(Log::LogLevel::Info, "Shader change noticed");
		HotReloadShaders(gfxDevice, &dxcRes, swapchain);
	}*/

	// imgui frame init
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	{
		// Start the Dear ImGui frame
		{
			ImGui::Begin("Frame stats!");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / inspector.io->Framerate, inspector.io->Framerate);
			ImGui::End();
		}
	}
	SubmitPasses(_commandList, gfxDevice, swapchain, frameSync, inspector, 
		camera, resources.pipelines[0],
		resources.pipelines[1], resources.pipelines[2], model);

	// present the Frame
	DX_ASSERT(swapchain._swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	MoveToNextFrame(gfxDevice, swapchain, frameSync);
}

void Scene::HotReloadShaders(GfxDevice& gfxDevice, DXCRes* dxcRes, Swapchain& swapchain)
{
	for (auto& shader : resources.shaders) shader.Release();
	for (auto& pipeline : resources.pipelines) pipeline.Release();

	resources.shaders.clear();
	resources.pipelines.clear();

	
}