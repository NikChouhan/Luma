
#include "ForwardPBRPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Graphics/PushConstants.h"
#include "Renderer/Core/RenderCtx.h"
#include "Graphics/RHI/RHI.h"

void ForwardPBRPass::Init()
{
	/* it will be a simple pixel shading pass that first colors the pixel with the textures
	 * and then depending on the cluster its present in, adds the lights
	 * contribution. A vertex and a pixel shader is all that is needed
	*/

	// raster pipeline and the shaders
	{
		ShaderHandle vsHandle = RHI::CreateShader({
			.path = L"../../../../shaders/Forward/ForwardPBR.hlsl",
			.entryPoint = L"VSMain",
			.target = L"vs_6_7",
			.stage = RHIShaderStage::VERTEX });

		ShaderHandle psHandle = RHI::CreateShader({
			.path = L"../../../../shaders/Forward/ForwardPBR.hlsl",
			.entryPoint = L"PSMain",
			.target = L"ps_6_7",
			.stage = RHIShaderStage::PIXEL });

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
			.inputAttributes = kInputAttributes });
	}
}

void ForwardPBRPass::Execute(RenderCtx& ctx, const Scene& scene)
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

			i32 albedoIndex = materials.at(materialIndex).albedoTexture.index;
			i32 normalIndex = materials.at(materialIndex).normalTexture.index;
			i32 metallicRoughnessIndex = materials.at(materialIndex).metallicRoughnessTexture.index;
			i32	emissiveIndex = materials.at(materialIndex).emissiveTexture.index;


			DirectX::XMMATRIX world = mesh.transform * renderObj.transform;
			DirectX::XMMATRIX wvp = world * viewProj;

			pushConstants.worldViewProj = wvp;

			pushConstants.albedoIndex = albedoIndex;
			pushConstants.normalIndex = normalIndex;
			pushConstants.metallicRoughnessIndex = metallicRoughnessIndex;
			pushConstants.emissiveIndex = emissiveIndex;

			cmdList->SetGraphicsPushConstants(&pushConstants, sizeof(ForwardPBRPassRootConstants) / 4, 0);
			cmdList->DrawIndexed(mesh.indexCount, 1, mesh.startIndex, mesh.baseVertex, 0);
		}
	}
}