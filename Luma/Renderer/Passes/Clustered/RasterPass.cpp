//#include "RasterPass.h"
//
//#include "scene.h"
//#include "Core/Camera.h"
//#include "Graphics/PushConstants.h"
//#include "Graphics/RHI/D3D12/Shader.h"
//#include "Renderer/Core/PipelineCache.h"
//#include "Renderer/Core/RenderContext.h"
//
//void RasterPass::Init()
//{
//
//	/* it will be a simple pixel shading pass that first colors the pixel with the textures
//	 * and then depending on the cluster its present in, adds the lights
//	 * contribution. A vertex and a pixel shader is all that is needed
//	*/
//
//	// raster pipeline and the shaders
//	{
//		ShaderDesc vsDesc{
//		.shaderPath = L"../../../../shaders/model.hlsl",
//		.pEntryPoint = L"VSMain",
//		.pTarget = L"vs_6_7",
//		.type = Type::VERTEX };
//		ShaderH vsHandle = pipelineCache->LoadShader(vsDesc, "RasterPassVertexShader");
//
//		ShaderDesc psDesc{
//			.shaderPath = L"../../../../shaders/model.hlsl",
//			.pEntryPoint = L"PSMain",
//			.pTarget = L"ps_6_7",
//			.type = Type::PIXEL };
//		ShaderH psHandle = pipelineCache->LoadShader(psDesc, "RasterPassPixelShader");
//
//		GraphicsPipelineDesc desc{
//			.vertexShader = vsHandle,
//			.pixelShader = psHandle,
//			.blendMode = BlendMode::NON_TRANSPARENT,
//			.depthMode = DepthMode::READ_ONLY,
//			.depthFunc = DepthFunc::GREATER_EQUAL,
//			.rasterMode = RasterMode::SOLID_NONE_CULL,
//			.topology = Topology::TRIANGLES,
//			.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
//			.dsvFormat = DXGI_FORMAT_D32_FLOAT
//		};
//		RasterPipelineHandle_ = pipelineCache->CreatePipeline(desc, "RasterPipeline");
//	}
//	// light shading pipelines, shaders, resources
//	{
//		/*ResourceHandle GlobalLightsStructuredBufferHandle = resourceManager->GetResourceHandleByName("GlobalLightsStructuredBuffer");
//		ResourceHandle LightListCounterBufferHandle = resourceManager->GetResourceHandleByName("LightListCounterBuffer");
//		ResourceHandle LightIndexListTextureHandle = resourceManager->GetResourceHandleByName("LightIndexListTexture");
//		ResourceHandle LightIndicesBufferHandle = resourceManager->GetResourceHandleByName("LightIndicesBuffer");
//
//		Resource* lightListCounterBuffer = resourceManager_->GetResource(LightListCounterBufferHandle);
//		Resource* lightIndexListTexture = resourceManager_->GetResource(LightIndexListTextureHandle);
//		Resource* lightIndicesBuffer = resourceManager_->GetResource(LightIndicesBufferHandle);
//		Resource* globalLightsStructuredBuffer = resourceManager_->GetResource(GlobalLightsStructuredBufferHandle);
//
//		const u32 lightListCounterBufferSRVIndex = std::get_if<Buffer>(lightListCounterBuffer)->AsShaderResourceView()->heapIndex.value();
//		const u32 lightListTextureSRVIndex = std::get_if<Texture>(lightIndexListTexture)->srvIndex.value();
//		const u32 lightIndicesBufferSRVIndex = std::get_if<Buffer>(lightIndicesBuffer)->AsShaderResourceView()->heapIndex.value();
//		const u32 globalLightStructuredBufferSRVIndex = std::get_if<Buffer>(globalLightsStructuredBuffer)->AsShaderResourceView()->heapIndex.value();
//
//		pushConstants.lightListCounterBufferSRVIndex = lightListCounterBufferSRVIndex;
//		pushConstants.lightListTextureSRVIndex = lightListTextureSRVIndex;
//		pushConstants.lightIndicesBufferSRVIndex = lightIndicesBufferSRVIndex;
//		pushConstants.globalLightStructuredBufferSRVIndex = globalLightStructuredBufferSRVIndex;*/
//
//		ResourceHandle ClusterResourceHandle = resourceManager->GetResourceHandleByName("Clusters");
//		Resource* cluster = resourceManager_->GetResource(ClusterResourceHandle);
//		const u32 clusterIndex = std::get_if<Buffer>(cluster)->AsShaderResourceView()->heapIndex.value();
//		pushConstants.clusterIndex = clusterIndex;
//
//		pushConstants.ScreenResolution = SM::Vector2{ f32(GlobalStorage::g_LumaConstants.width), f32(GlobalStorage::g_LumaConstants.height)};
//		pushConstants.depthSRVIndex = GlobalStorage::depthSRVIndex;
//	}
//}
//
//void RasterPass::Execute(RenderContext& ctx, const Scene& scene)
//{
//	auto cmdList = ctx.cmdList_;
//	Pipeline* pipeline = pipelineCache_->GetPipeline(RasterPipelineHandle_);
//
//	// setup frame state
//	cmdList->SetPipelineState(pipeline->pso.Get());
//
//	// already set in renderer.BeginFrame();
//	/*ID3D12DescriptorHeap* ppHeaps[] = { resourceManager_->GetBindlessHeap().Get() };
//	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);*/
//
//	cmdList->SetGraphicsRootSignature(pipeline->rootSign.Get());
//
//	cmdList->RSSetViewports(1, &ctx.viewport);
//	cmdList->RSSetScissorRects(1, &ctx.scissorRect);
//
//	cmdList->OMSetRenderTargets(1, &ctx.currentRtv, 
//		FALSE, &ctx.currentDsv);
//
//	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//	const Camera& cam = scene.GetCamera();
//	DirectX::XMMATRIX view = cam.view;
//	DirectX::XMMATRIX proj = cam.projection;
//	pushConstants.cameraPosition = cam.pos;
//
//	DirectX::XMMATRIX viewProj = cam.view * cam.projection;
//	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(nullptr, viewProj);
//
//	for (const auto& renderObj : scene.GetRenderObjects())
//	{
//		Model* model = renderObj.model;
//		if (!model) continue;
//
//		Resource* vbRes = resourceManager_->GetResource(model->GetVertexBuffer());
//		Resource* ibRes = resourceManager_->GetResource(model->GetIndexBuffer());
//
//		if (!vbRes || !ibRes) continue;
//
//		const VertexBufferView* vbView = std::get_if<Buffer>(vbRes)->AsVertexBuffer();
//		const IndexBufferView* ibView = std::get_if<Buffer>(ibRes)->AsIndexBuffer();
//
//		cmdList->IASetVertexBuffers(0, 1, &vbView->view);
//		cmdList->IASetIndexBuffer(&ibView->view);
//
//		for (const auto& mesh : model->GetSubMeshes())
//		{
//			int materialIndex = mesh.materialIndex;
//			const std::vector materials = model->GetMaterials();
//
//			ResourceHandle albedoHandle = materials.at(materialIndex).albedoTexture;
//			ResourceHandle normalHandle = materials.at(materialIndex).normalTexture;
//			ResourceHandle metallicRoughnessHandle = materials.at(materialIndex).metallicRoughnessTexture;
//			ResourceHandle emissiveHandle = materials.at(materialIndex).emissiveTexture;
//
//			i32 albedoIndex{-1}, normalIndex{-1}, metallicRoughnessIndex{-1}, emissiveIndex{-1};
//
//			Resource* albedo = resourceManager_->GetResource(albedoHandle);
//			if (albedo) albedoIndex = std::get_if<Texture>(albedo)->srvIndex.value();
//
//			Resource* normal = resourceManager_->GetResource(normalHandle);
//			if (normal) normalIndex = std::get_if<Texture>(normal)->srvIndex.value();
//
//			Resource* metallicRoughness = resourceManager_->GetResource(metallicRoughnessHandle);
//			if (metallicRoughness) metallicRoughnessIndex = std::get_if<Texture>(metallicRoughness)->srvIndex.value();
//
//			Resource* emissive = resourceManager_->GetResource(emissiveHandle);
//			if (emissive) emissiveIndex = std::get_if<Texture>(emissive)->srvIndex.value();
//
//			DirectX::XMMATRIX world = mesh.transform * renderObj.transform;
//			DirectX::XMMATRIX wvp = world * viewProj;
//
//			pushConstants.worldViewProj = (wvp);
//			//pushConstants.worldMatrix = (world);
//			pushConstants.inverseViewProj = invViewProj;
//
//			pushConstants.albedoIndex = albedoIndex;
//			pushConstants.normalIndex = normalIndex;
//			pushConstants.metallicRoughnessIndex = metallicRoughnessIndex;
//			pushConstants.emissiveIndex = emissiveIndex;
//
//			cmdList->SetGraphicsRoot32BitConstants(0, sizeof(RasterPassRootConstants) / 4, &pushConstants, 0);
//
//			cmdList->DrawIndexedInstanced(
//				mesh.indexCount,
//				1,
//				mesh.startIndex,
//				mesh.baseVertex,
//				0
//			);
//		}
//	}
//}