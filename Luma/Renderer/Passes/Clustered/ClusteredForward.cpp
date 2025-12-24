#include "ClusteredForward.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderContext.h"

constexpr auto cluster_number = (16*9*24);

void ClusteredForward::Init(ResourceManager* resourceManager, PipelineCache* pipelineCache)
{
	// pass 1
	// compute AABB
	{
		BufferCreateInfo bufferCreateInfo{
		.desc = StructuredBufferDesc{.data = nullptr,
			.elementCount = cluster_number,
			.elementStride = sizeof(SM::Vector4),
			.createSRV = true,	// will be used in the next shader for Reading
			.createUAV = true},
		.usage = BufferUsage::DEFAULT,
		.bufferResourceViewFlags = BufferViewFlags::UAV | BufferViewFlags::SRV,
		.keepMapped = true,
		.debugName = L"Clusters",
		.bindlessHeap = resourceManager->GetBindlessHeap().Get() };

		ClusterResourceHandle = resourceManager->CreateResource(bufferCreateInfo, "Clusters");

		ShaderHandle clusteredComp = pipelineCache->LoadShader({
		.shaderPath = L"../../../../shaders/Clustered/ComputeAABB.hlsl",
		.pEntryPoint = L"ClusterMain",
		.pTarget = L"cs_6_7",
		.type = Type::COMPUTE });
		ComputePipelineDesc computePipelineAABBdesc{
		.computeShader = clusteredComp };
		ComputeAABBPipeline = pipelineCache->CreatePipeline(computePipelineAABBdesc, "ClusteredAABBCompute");
	}

	// pass 2
	// TODO: mark active clusters
	// pass 3
	// for each (active) cluster, loop over all the lights and test Cluster AABB with
	// Light sphere for intersection and assign light indices
	{
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
	}
}

void ClusteredForward::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cmdList_;

	Resource* cluster = resourceManager_->GetResource(ClusterResourceHandle);
	const u32 clusterIndex = std::get_if<Texture>(cluster)->uavIndex.value();
	// pass 1 to compute AABB
	{
		Pipeline* pipeline = pipelineCache_->GetPipeline(ComputeAABBPipeline);

		cmdList->SetPipelineState(pipeline->pso.Get());
		cmdList->SetComputeRootSignature(pipeline->rootSign.Get());

		const Camera& cam = scene.GetCamera();
		DirectX::XMMATRIX view = cam._view;
		DirectX::XMMATRIX proj = cam._projection;


		ComputeAABBData pushConstants{};
		pushConstants.inverseProj = DirectX::XMMatrixInverse(nullptr, proj);

		pushConstants.clusterInputData[0] = 16;
		pushConstants.clusterInputData[1] = 9;
		pushConstants.clusterInputData[2] = 24;
		pushConstants.zNear = cam._near;

		pushConstants.screenDimensions[0] = GlobalStorage::g_LumaConstants.width;
		pushConstants.screenDimensions[1] = GlobalStorage::g_LumaConstants.height;
		pushConstants.zFar = cam._far;
		pushConstants.clusterUAVIndex = clusterIndex;

		// TODO: use this instead of bindless for testing idk? A nice alternative
		// cmdList->SetComputeRootUnorderedAccessView()

		cmdList->SetComputeRoot32BitConstants(0, sizeof(ComputeAABBData) / 4,
			&pushConstants, 0);

		cmdList->Dispatch(clusterSizeXYZ[0], clusterSizeXYZ[1], clusterSizeXYZ[2]);
	}
	// 
	{
		Pipeline* pipeline = pipelineCache_->GetPipeline(LightAssignClusterPipeline);

		cmdList->SetPipelineState(pipeline->pso.Get());
		cmdList->SetComputeRootSignature(pipeline->rootSign.Get());

		Resource* lightListCounterBuffer = resourceManager_->GetResource(LightListCounterBufferHandle);
		Resource* lightIndexListTexture = resourceManager_->GetResource(LightIndexListTextureHandle);
		Resource* lightIndicesBuffer = resourceManager_->GetResource(LightIndicesBufferHandle);

		const u32 lightListCounterBufferUAVIndex = std::get_if<Buffer>(lightListCounterBuffer)->AsUnorderedAccessView()->heapIndex.value();
		const u32 lightListTextureUAVIndex = std::get_if<Texture>(lightIndexListTexture)->uavIndex.value();
		const u32 lightIndicesBufferUAVIndex = std::get_if<Buffer>(lightIndicesBuffer)->AsUnorderedAccessView()->heapIndex.value();

		LightAssignCluster pushConstants{};
		pushConstants.clusterInputData[0] = clusterSizeXYZ[0];
		pushConstants.clusterInputData[1] = clusterSizeXYZ[1];
		pushConstants.clusterInputData[2] = clusterSizeXYZ[2];

		pushConstants.clusterUAVIndex = clusterIndex;
		pushConstants.LightListTextureUAVIndex = lightListTextureUAVIndex;
		pushConstants.LightCounterUAVIndex = lightListCounterBufferUAVIndex;
		pushConstants.LightIndexBufferUAVIndex = lightIndicesBufferUAVIndex;
		pushConstants.lightCount = max_lights;

		cmdList->SetComputeRoot32BitConstants(0, sizeof(LightAssignCluster) / 4, &pushConstants, 0);

		cmdList->Dispatch(clusterSizeXYZ[0], clusterSizeXYZ[1], clusterSizeXYZ[2]);
	}
}
