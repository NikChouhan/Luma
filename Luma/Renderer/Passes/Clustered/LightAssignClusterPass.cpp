#include "LightAssignClusterPass.h"

#include "GenerateLights.h"
#include "scene.h"
#include "Core/Camera.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderContext.h"


void LightAssignClusterPass::Init(ResourceManager* resourceManager, PipelineCache* pipelineCache)
{
	// for each (active) cluster, loop over all the lights and test Cluster AABB with
	// Light sphere for intersection and assign light indices

	// first generate a structured buffer for lights
	{
		std::vector<Light> allLights = GenerateLights(max_lights);

		BufferCreateInfo globalLightsBufferDesc
		{
			.desc = StructuredBufferDesc{.data = allLights.data(), .elementCount = u32(allLights.size()), .elementStride = sizeof(Light), .createSRV = true, .createUAV = true},
			.usage = BufferUsage::UPLOAD,
			.bufferResourceViewFlags = BufferViewFlags::SRV | BufferViewFlags::UAV,
			.keepMapped = true,
			.debugName = L"GlobalLightBuffer",
			.bindlessHeap = resourceManager->GetBindlessHeap().Get()
		};

		GlobalLightsStructuredBufferHandle = resourceManager->CreateResource(globalLightsBufferDesc, "GlobalLightBuffer");
	}

	// resources
	{
		// resource 1
		BufferCreateInfo lightListCounterBufferDesc{
		.desc = StructuredBufferDesc {
			.data = nullptr,
			.elementCount = cluster_number,
			.elementStride = sizeof(u32),
			.createSRV = true,
			.createUAV = true },
		.usage = BufferUsage::DEFAULT,
		.bufferResourceViewFlags = BufferViewFlags::UAV | BufferViewFlags::SRV,
		.keepMapped = true,
		.debugName = L"LightListCounterBuffer",
		.bindlessHeap = resourceManager->GetBindlessHeap().Get() };

		LightListCounterBufferHandle = resourceManager->CreateResource(lightListCounterBufferDesc, "LightListCounterBuffer");

		// resource 2
		TextureCreateInfo textureCreateInfo{
		.desc = {
			.width = clusterSizeXYZ[0],
			.height = clusterSizeXYZ[1],
			.depth = clusterSizeXYZ[2],
			.texPixelSize = 0, .mipLevels = 1, .arraySize = 1,
			.format = DXGI_FORMAT_R32G32_UINT,
			.viewFlags = TextureViewFlags::SRV | TextureViewFlags::UAV,
			.createMipUAVs = false,
			.initialData = nullptr
		},
		.debugName = L"GlobalLightIndexListTexture",
		.usage = TextureUsage::UPLOAD,
		.heap = resourceManager->GetBindlessHeap().Get() };

		LightIndexListTextureHandle = resourceManager->CreateResource(textureCreateInfo, "GlobalLightIndexListTexture");

		// resource 3
		BufferCreateInfo lightIndicesBufferDesc{
		.desc = StructuredBufferDesc {
			.data = nullptr,
			.elementCount = maxLightIndices,
			.elementStride = sizeof(u32),
			.createSRV = true,
			.createUAV = true },
		.usage = BufferUsage::DEFAULT,
		.bufferResourceViewFlags = BufferViewFlags::UAV | BufferViewFlags::SRV,
		.keepMapped = true,
		.debugName = L"LightIndicesBuffer",
		.bindlessHeap = resourceManager->GetBindlessHeap().Get() };

		LightIndicesBufferHandle = resourceManager->CreateResource(lightIndicesBufferDesc, "LightIndicesBuffer");
	}

	ShaderHandle lightAssignCluster = pipelineCache->LoadShader({
	.shaderPath = L"../../../../shaders/Clustered/LightAssignCluster.hlsl",
	.pEntryPoint = L"CSLightAssign",
	.pTarget = L"cs_6_7",
	.type = Type::COMPUTE });
	LightAssignClusterPipeline = pipelineCache->CreatePipeline({ .computeShader = lightAssignCluster },
		"LightAssignCluster");

	clusterSizeXYZ[0] = 16;
	clusterSizeXYZ[1] = 9;
	clusterSizeXYZ[2] = 24;

	ClusterResourceHandle = resourceManager->GetResourceHandleByName("Clusters");
}

void LightAssignClusterPass::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cmdList_;

	Resource* cluster = resourceManager_->GetResource(ClusterResourceHandle);
	const u32 clusterIndex = std::get_if<Texture>(cluster)->uavIndex.value();

	Pipeline* pipeline = pipelineCache_->GetPipeline(LightAssignClusterPipeline);

	cmdList->SetPipelineState(pipeline->pso.Get());
	cmdList->SetComputeRootSignature(pipeline->rootSign.Get());

	Resource* lightListCounterBuffer = resourceManager_->GetResource(LightListCounterBufferHandle);
	Resource* lightIndexListTexture = resourceManager_->GetResource(LightIndexListTextureHandle);
	Resource* lightIndicesBuffer = resourceManager_->GetResource(LightIndicesBufferHandle);
	Resource* globalLightsStructuredBuffer = resourceManager_->GetResource(GlobalLightsStructuredBufferHandle);

	const u32 lightListCounterBufferUAVIndex = std::get_if<Buffer>(lightListCounterBuffer)->AsUnorderedAccessView()->heapIndex.value();
	const u32 lightListTextureUAVIndex = std::get_if<Texture>(lightIndexListTexture)->uavIndex.value();
	const u32 lightIndicesBufferUAVIndex = std::get_if<Buffer>(lightIndicesBuffer)->AsUnorderedAccessView()->heapIndex.value();
	const u32 globalLightStructuredBufferUAVIndex = std::get_if<Buffer>(globalLightsStructuredBuffer)->AsUnorderedAccessView()->heapIndex.value();

	LightAssignCluster pushConstants{};
	pushConstants.clusterInputData[0] = clusterSizeXYZ[0];
	pushConstants.clusterInputData[1] = clusterSizeXYZ[1];
	pushConstants.clusterInputData[2] = clusterSizeXYZ[2];

	pushConstants.clusterUAVIndex = clusterIndex;
	pushConstants.LightListTextureUAVIndex = lightListTextureUAVIndex;
	pushConstants.LightCounterUAVIndex = lightListCounterBufferUAVIndex;
	pushConstants.LightIndexBufferUAVIndex = lightIndicesBufferUAVIndex;
	pushConstants.lightCount = max_lights;

	pushConstants.globalLightStructuredBufferUAVIndex = globalLightStructuredBufferUAVIndex;

	cmdList->SetComputeRoot32BitConstants(0, sizeof(LightAssignCluster) / 4, &pushConstants, 0);

	cmdList->Dispatch(clusterSizeXYZ[0], clusterSizeXYZ[1], clusterSizeXYZ[2]);
}
