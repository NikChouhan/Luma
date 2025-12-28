#include "GeometryPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Graphics/PushConstants.h"
#include "Graphics/D3D12/Shader.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderContext.h"

void GeometryPass::Init(ResourceManager* resourceManager, PipelineCache* pipelineCache)
{
	resourceManager_ = resourceManager;
	pipelineCache_ = pipelineCache;

	// create resources (texture[views]/buffer[views]) and pipelines
	ShaderDesc vsDesc{
	.shaderPath = L"../../../../shaders/depth_pass.hlsl",
	.pEntryPoint = L"DepthVS",
	.pTarget = L"vs_6_7",
	.type = Type::VERTEX };
	ShaderHandle vsHandle = pipelineCache->LoadShader(vsDesc, "VertexShader");

	GraphicsPipelineDesc desc{
		.vertexShader = vsHandle,
		.pixelShader = g_invalidShaderHandle,
		.blendMode = BlendMode::NON_TRANSPARENT,
		.depthMode = DepthMode::READ_WRITE,
		.depthFunc = DepthFunc::LESS,
		.rasterMode = RasterMode::SOLID_NONE_CULL,
		.topology = Topology::TRIANGLES,
		.rtvFormat = DXGI_FORMAT_UNKNOWN,
		.dsvFormat = DXGI_FORMAT_D32_FLOAT
	};
	pipelineHandle_ = pipelineCache->CreatePipeline(desc, "Geometry Pipeline");
}

void GeometryPass::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cmdList_;
	Pipeline* pipeline = pipelineCache_->GetPipeline(pipelineHandle_);

	// setup frame state
	cmdList->SetPipelineState(pipeline->pso.Get());

	// already set in renderer.BeginFrame();
	/*ID3D12DescriptorHeap* ppHeaps[] = { resourceManager_->GetBindlessHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);*/

	cmdList->SetGraphicsRootSignature(pipeline->rootSign.Get());

	cmdList->RSSetViewports(1, &ctx.viewport);
	cmdList->RSSetScissorRects(1, &ctx.scissorRect);

	cmdList->ClearDepthStencilView(ctx.currentDsv, D3D12_CLEAR_FLAG_DEPTH,
		1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(0, nullptr,
		FALSE, &ctx.currentDsv);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const Camera& cam = scene.GetCamera();
	DirectX::XMMATRIX view = cam._view;
	DirectX::XMMATRIX proj = cam._projection;

	for (const auto& renderObj : scene.GetRenderObjects())
	{
		Model* model = renderObj.model;
		if (!model) continue;

		Resource* vbRes = resourceManager_->GetResource(model->GetVertexBuffer());
		Resource* ibRes = resourceManager_->GetResource(model->GetIndexBuffer());

		if (!vbRes || !ibRes) continue;

		const VertexBufferView* vbView = std::get_if<Buffer>(vbRes)->AsVertexBuffer();
		const IndexBufferView* ibView = std::get_if<Buffer>(ibRes)->AsIndexBuffer();

		cmdList->IASetVertexBuffers(0, 1, &vbView->view);
		cmdList->IASetIndexBuffer(&ibView->view);

		for (const auto& mesh : model->GetSubMeshes())
		{
			DirectX::XMMATRIX world = mesh.transform * renderObj.transform;
			DirectX::XMMATRIX wvp = world * view * proj;

			DrawModel constants;
			constants.worldViewProj = (wvp);
			constants.worldMatrix = (world);

			cmdList->SetGraphicsRoot32BitConstants(0, sizeof(DepthPPBuffer) / 4,
				&constants, 0);

			cmdList->DrawIndexedInstanced(
				mesh.indexCount,
				1,
				mesh.startIndex,
				mesh.baseVertex,
				0
			);
		}
	}
}
