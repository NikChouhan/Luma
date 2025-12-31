#include "ComputeAABBPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderContext.h"

void ComputeAABBPass::Init(ResourceManager* resourceManager, PipelineCache* pipelineCache)
{
	resourceManager_ = resourceManager;
	pipelineCache_ = pipelineCache;

	// compute AABB pass
	{
		BufferCreateInfo bufferCreateInfo{
		.desc = StructuredBufferDesc{.data = nullptr,
			.elementCount = cluster_number,
			.elementStride = sizeof(SM::Vector4),
			.createSRV = true,	// will be used in the next shader for Reading
			.createUAV = true},
		.usage = BufferUsage::DEFAULT,
		.bufferResourceViewFlags = BufferViewFlags::UAV | BufferViewFlags::SRV,
		.keepMapped = false,
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

	clusterSizeXYZ[0] = 16;
	clusterSizeXYZ[1] = 9;
	clusterSizeXYZ[2] = 24;
}

void ComputeAABBPass::Execute(RenderContext& ctx, const Scene& scene)
{
	auto cmdList = ctx.cmdList_;

	Resource* cluster = resourceManager_->GetResource(ClusterResourceHandle);
	const u32 clusterIndex = std::get_if<Buffer>(cluster)->AsUnorderedAccessView()->heapIndex.value();

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
