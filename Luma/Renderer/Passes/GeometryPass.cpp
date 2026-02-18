#include "GeometryPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Graphics/PushConstants.h"
#include "Renderer/Core/RenderCtx.h"
#include "Graphics/RHI/RHI.h"

void GeometryPass::Init()
{
	// create resources (texture[views]/buffer[views]) and pipelines

	// rhi impl
	RHIShaderDesc vsDesc;
	ShaderHandle vsHandle = RHI::CreateShader(
		{
		.path = L"../../../../shaders/Geometry/DepthPrePass.hlsl",
		.entryPoint = L"DepthVS",
		.target = L"vs_6_7",
		.stage = RHIShaderStage::VERTEX }
		);
	pipelineHandle = RHI::CreateGraphicsPipeline(
		{
		.vs = vsHandle,
		.ps = g_invalidShaderHandle,
		.blend = RHIBlendMode::NON_TRANSPARENT,
		.depthFunc = RHIDepthFunc::GREATER,
		.depthMode = RHIDepthMode::WRITE,
		.rasterMode = RHIRasterMode::NONE,
		.topology = RHITopology::TRIANGLE_LIST,
		.colorFormats = {RHIFormat::R8G8B8A8_UNORM},
		.depthFormat = RHIFormat::D32_FLOAT,
		.inputBindings = kBindingdescs,
		.inputAttributes = kInputAttributes }
		);
}

void GeometryPass::Execute(RenderCtx& ctx, const Scene& scene)
{
	auto cmdList = ctx.cl;
	// setup frame state
	cmdList->SetPipeline(pipelineHandle);
	cmdList->SetViewport({});
	cmdList->SetScissor({});
	// supply the render targets here. I will be using directly from the backend so pass empty for now
	cmdList->BeginRendering({}, g_invalidTextureHandle);

	//cmdList->TextureBarrier(g_invalidTextureHandle, RHIResourceState::DEPTH_READ, RHIResourceState::DEPTH_WRITE);

	const Camera& cam = scene.GetCamera();

	DirectX::XMMATRIX viewProj = cam.view * cam.projection;
	for (const auto& renderObj : scene.GetRenderObjects())
	{
		Model* model = renderObj.model;
		if (!model) continue;

		cmdList->BindVertexBuffer(model->GetVertexBuffer());
		cmdList->BindIndexBuffer(model->GetIndexBuffer());

		for (const auto& mesh : model->GetSubMeshes())
		{
			DirectX::XMMATRIX world = mesh.transform * renderObj.transform;
			DirectX::XMMATRIX wvp = world * viewProj;

			DepthPassRootConstants constants;
			constants.worldViewProj = (wvp);
			constants.worldMatrix = (world);

			cmdList->SetGraphicsPushConstants(&constants, sizeof(DepthPassRootConstants)/4, 0);
			cmdList->DrawIndexed(mesh.indexCount, 1, mesh.startIndex, mesh.baseVertex, 0);
		}
	}
}