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
	.shaderPath = L"../../../../shaders/model.hlsl",
	.pEntryPoint = L"VSMain",
	.pTarget = L"vs_6_7",
	.type = Type::VERTEX };
	ShaderHandle vsHandle = pipelineCache->LoadShader(vsDesc, "VertexShader");

	// create resources (texture[views]/buffer[views]) and pipelines
	ShaderDesc psDesc{
	.shaderPath = L"../../../../shaders/model.hlsl",
	.pEntryPoint = L"PSMain",
	.pTarget = L"ps_6_7",
	.type = Type::PIXEL };

	ShaderHandle psHandle = pipelineCache->LoadShader(psDesc, "PixelShader");

	pipelineHandle_ = pipelineCache->CreatePipeline(
		{
		.pipelineType = PipelineType::GRAPHICS,
		.rootSignType = RootSignDesc::RSType::RENDER,
		.shaders = {vsHandle, psHandle},
		.enableDepthTest = true,
		.enableStencilTest = true,
		.isDepthPrePass = false},
		"GeometryPipeline");
}

void GeometryPass::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cmdList_;
	Pipeline* pipeline = pipelineCache_->GetPipeline(pipelineHandle_);

	// setup frame state
	cmdList->SetPipelineState(pipeline->pipelineState.Get());

	// already set in renderer.BeginFrame();
	/*ID3D12DescriptorHeap* ppHeaps[] = { resourceManager_->GetBindlessHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);*/

	cmdList->SetGraphicsRootSignature(pipeline->rootSignature.Get());

	cmdList->RSSetViewports(1, &ctx.viewport);
	cmdList->RSSetScissorRects(1, &ctx.scissorRect);

	cmdList->OMSetRenderTargets(1, &ctx.currentRtv, FALSE, &ctx.currentDsv);
	cmdList->ClearDepthStencilView(ctx.currentDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
			constants.worldViewProj = DirectX::XMMatrixTranspose(wvp);
			constants.worldMatrix = DirectX::XMMatrixTranspose(world);

			/* TODO: haven't filled most of the structs, too late in the night :/
			* need to fill asap
			*/

			cmdList->SetGraphicsRoot32BitConstants(0, sizeof(DrawModel) / 4, &constants, 0);

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
