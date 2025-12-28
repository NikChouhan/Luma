#include "RasterPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Graphics/PushConstants.h"
#include "Graphics/D3D12/Shader.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderContext.h"

void RasterPass::Init(ResourceManager* resourceManager, PipelineCache* pipelineCache)
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

	ShaderDesc psDesc{
		.shaderPath = L"../../../../shaders/model.hlsl",
		.pEntryPoint = L"PSMain",
		.pTarget = L"ps_6_7",
		.type = Type::PIXEL };
	ShaderHandle psHandle = pipelineCache->LoadShader(psDesc, "PixelShader");

	GraphicsPipelineDesc desc{
		.vertexShader = vsHandle,
		.pixelShader = psHandle,
		.blendMode = BlendMode::NON_TRANSPARENT,
		.depthMode = DepthMode::READ_ONLY,
		.depthFunc = DepthFunc::EQUAL,
		.rasterMode = RasterMode::SOLID_NONE_CULL,
		.topology = Topology::TRIANGLES,
		.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
		.dsvFormat = DXGI_FORMAT_D32_FLOAT
	};
	RasterPipelineHandle_ = pipelineCache->CreatePipeline(desc, "RasterPipeline");
}

void RasterPass::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cmdList_;
	Pipeline* pipeline = pipelineCache_->GetPipeline(RasterPipelineHandle_);

	// setup frame state
	cmdList->SetPipelineState(pipeline->pso.Get());

	// already set in renderer.BeginFrame();
	/*ID3D12DescriptorHeap* ppHeaps[] = { resourceManager_->GetBindlessHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);*/

	cmdList->SetGraphicsRootSignature(pipeline->rootSign.Get());

	cmdList->RSSetViewports(1, &ctx.viewport);
	cmdList->RSSetScissorRects(1, &ctx.scissorRect);

	cmdList->OMSetRenderTargets(1, &ctx.currentRtv, 
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
			int materialIndex = mesh.materialIndex;
			const std::vector materials = model->GetMaterials();

			ResourceHandle albedoHandle = materials.at(materialIndex).albedoTexture;
			ResourceHandle normalHandle = materials.at(materialIndex).normalTexture;
			ResourceHandle metallicRoughnessHandle = materials.at(materialIndex).metallicRoughnessTexture;
			ResourceHandle emissiveHandle = materials.at(materialIndex).emissiveTexture;

			i32 albedoIndex{-1}, normalIndex{-1}, metallicRoughnessIndex{-1}, emissiveIndex{-1};

			Resource* albedo = resourceManager_->GetResource(albedoHandle);
			if (albedo) albedoIndex = std::get_if<Texture>(albedo)->srvIndex.value();

			Resource* normal = resourceManager_->GetResource(normalHandle);
			if (normal) normalIndex = std::get_if<Texture>(normal)->srvIndex.value();

			Resource* metallicRoughness = resourceManager_->GetResource(metallicRoughnessHandle);
			if (metallicRoughness) metallicRoughnessIndex = std::get_if<Texture>(metallicRoughness)->srvIndex.value();

			Resource* emissive = resourceManager_->GetResource(emissiveHandle);
			if (emissive) emissiveIndex = std::get_if<Texture>(emissive)->srvIndex.value();

			DirectX::XMMATRIX world = mesh.transform * renderObj.transform;
			DirectX::XMMATRIX wvp = world * view * proj;

			DrawModel constants;
			constants.worldViewProj = (wvp);
			constants.worldMatrix = (world);
			constants.albedoIndex = albedoIndex;
			constants.normalIndex = normalIndex;
			constants.metallicRoughnessIndex = metallicRoughnessIndex;
			constants.emissiveIndex = emissiveIndex;

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