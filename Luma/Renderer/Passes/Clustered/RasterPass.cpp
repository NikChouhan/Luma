
#include "RasterPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Graphics/PushConstants.h"
#include "Renderer/Core/RenderContext.h"
#include "Graphics/RHI/RHI.h"

void RasterPass::Init()
{
	/* it will be a simple pixel shading pass that first colors the pixel with the textures
	 * and then depending on the cluster its present in, adds the lights
	 * contribution. A vertex and a pixel shader is all that is needed
	*/

	// raster pipeline and the shaders
	{
		ShaderHandle vsHandle = RHI::CreateShader({
			.path = L"../../../../shaders/model.hlsl",
			.entryPoint = L"VSMain",
			.target = L"vs_6_7",
			.stage = RHIShaderStage::VERTEX
			});

		ShaderHandle psHandle = RHI::CreateShader({
			.path = L"../../../../shaders/model.hlsl",
			.entryPoint = L"PSMain",
			.target = L"ps_6_7",
			.stage = RHIShaderStage::PIXEL
			});

		rasterPipelineHandle = RHI::CreateGraphicsPipeline({
			.vs = vsHandle,
			.ps = psHandle,
			.blend = RHIBlendMode::NON_TRANSPARENT,
			.depthFunc = RHIDepthFunc::GEQUAL,
			.depthMode = RHIDepthMode::READ,
			.rasterMode = RHIRasterMode::NONE,
			.topology = RHITopology::TRIANGLE_LIST,
			.colorFormats = {RHIFormat::R8G8B8A8_UNORM},
			.depthFormat = RHIFormat::D32_FLOAT,
			.inputBindings = kBindingdescs,
			.inputAttributes = kInputAttributes
			});
	}

	// light shading pipelines, shaders, resources
	{
		/*ResourceHandle GlobalLightsStructuredBufferHandle = resourceManager->GetResourceHandleByName("GlobalLightsStructuredBuffer");
		ResourceHandle LightListCounterBufferHandle = resourceManager->GetResourceHandleByName("LightListCounterBuffer");
		ResourceHandle LightIndexListTextureHandle = resourceManager->GetResourceHandleByName("LightIndexListTexture");
		ResourceHandle LightIndicesBufferHandle = resourceManager->GetResourceHandleByName("LightIndicesBuffer");

		Resource* lightListCounterBuffer = resourceManager_->GetResource(LightListCounterBufferHandle);
		Resource* lightIndexListTexture = resourceManager_->GetResource(LightIndexListTextureHandle);
		Resource* lightIndicesBuffer = resourceManager_->GetResource(LightIndicesBufferHandle);
		Resource* globalLightsStructuredBuffer = resourceManager_->GetResource(GlobalLightsStructuredBufferHandle);

		const u32 lightListCounterBufferSRVIndex = std::get_if<Buffer>(lightListCounterBuffer)->AsShaderResourceView()->heapIndex.value();
		const u32 lightListTextureSRVIndex = std::get_if<Texture>(lightIndexListTexture)->srvIndex.value();
		const u32 lightIndicesBufferSRVIndex = std::get_if<Buffer>(lightIndicesBuffer)->AsShaderResourceView()->heapIndex.value();
		const u32 globalLightStructuredBufferSRVIndex = std::get_if<Buffer>(globalLightsStructuredBuffer)->AsShaderResourceView()->heapIndex.value();

		pushConstants.lightListCounterBufferSRVIndex = lightListCounterBufferSRVIndex;
		pushConstants.lightListTextureSRVIndex = lightListTextureSRVIndex;
		pushConstants.lightIndicesBufferSRVIndex = lightIndicesBufferSRVIndex;
		pushConstants.globalLightStructuredBufferSRVIndex = globalLightStructuredBufferSRVIndex;*/

		//BufferHandle clusterBufferHandle = RHI::g_invalidBufferHandle; // TODO: Get from resource manager


		// TODO: Get actual screen resolution
		pushConstants.screenResolution = DirectX::XMFLOAT2(1920.0f, 1080.0f);

		//TextureHandle depthTextureHandle = g_invalidTextureHandle; // TODO: Get from resource manager
		//pushConstants.depthSRVIndex = RHI::GetBindlessReadIndex(depthTextureHandle);
	}
}

void RasterPass::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cl;

	// setup frame state
	cmdList->SetPipeline(rasterPipelineHandle);
	cmdList->SetViewport({});
	cmdList->SetScissor({});
	cmdList->BeginRendering({ g_invalidTextureHandle }, g_invalidTextureHandle);

	cmdList->TextureBarrier(g_invalidTextureHandle, RHIResourceState::DEPTH_WRITE, RHIResourceState::DEPTH_READ);

	const Camera& cam = scene.GetCamera();
	DirectX::XMMATRIX viewProj = cam.view * cam.projection;
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(nullptr, viewProj);

	pushConstants.cameraPosition = cam.pos;

	//pushConstants.clusterIndex = RHI::GetBindlessReadIndex(clusterBufferHandle);

	for (const auto& renderObj : scene.GetRenderObjects())
	{
		Model* model = renderObj.model;
		if (!model) continue;

		cmdList->BindVertexBuffer(model->GetVertexBuffer());
		cmdList->BindIndexBuffer(model->GetIndexBuffer());

		for (const auto& mesh : model->GetSubMeshes())
		{
			int materialIndex = mesh.materialIndex;
			const std::vector materials = model->GetMaterials();

			//TextureHandle albedoHandle = materials.at(materialIndex).albedoTexture;
			//TextureHandle normalHandle = materials.at(materialIndex).normalTexture;
			//TextureHandle metallicRoughnessHandle = materials.at(materialIndex).metallicRoughnessTexture;
			//TextureHandle emissiveHandle = materials.at(materialIndex).emissiveTexture;

			//i32 albedoIndex = RHI::GetBindlessReadIndex(albedoHandle);
			//i32 normalIndex = RHI::GetBindlessReadIndex(normalHandle);
			//i32 metallicRoughnessIndex = RHI::GetBindlessReadIndex(metallicRoughnessHandle);
			//i32 emissiveIndex = RHI::GetBindlessReadIndex(emissiveHandle);

			i32 albedoIndex = materials.at(materialIndex).albedoTexture.index;
			i32 normalIndex = materials.at(materialIndex).normalTexture.index;
			i32 metallicRoughnessIndex = materials.at(materialIndex).metallicRoughnessTexture.index;
			i32	emissiveIndex = materials.at(materialIndex).emissiveTexture.index;


			DirectX::XMMATRIX world = mesh.transform * renderObj.transform;
			DirectX::XMMATRIX wvp = world * viewProj;

			pushConstants.worldViewProj = wvp;
			pushConstants.inverseViewProj = invViewProj;

			pushConstants.albedoIndex = albedoIndex;
			pushConstants.normalIndex = normalIndex;
			pushConstants.metallicRoughnessIndex = metallicRoughnessIndex;
			pushConstants.emissiveIndex = emissiveIndex;

			cmdList->SetGraphicsPushConstants(&pushConstants, sizeof(RasterPassRootConstants) / 4, 0);
			cmdList->DrawIndexed(mesh.indexCount, 1, mesh.startIndex, mesh.baseVertex, 0);
		}
	}
}