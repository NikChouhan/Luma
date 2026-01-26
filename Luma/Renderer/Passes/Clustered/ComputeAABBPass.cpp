#include "ComputeAABBPass.h"

#include "scene.h"
#include "Core/Camera.h"
#include "Renderer/Core/RenderContext.h"

//void ComputeAABBPass::Init()
//{
//
//	BufferCreateInfo bufferCreateInfo{
//	.desc = StructuredBufferDesc{.data = nullptr,
//		.elementCount = cluster_number,
//		.elementStride = 2* sizeof(SM::Vector4),
//		.createSRV = true,	// will be used in the next shader for Reading
//		.createUAV = true},
//	.usage = BufferUsage::DEFAULT,
//	.bufferResourceViewFlags = BufferViewFlags::UAV | BufferViewFlags::SRV,
//	.keepMapped = false,
//	.debugName = L"Clusters",
//	.bindlessHeap = resourceManager->GetBindlessHeap().Get() };
//
//	ResourceHandle ClusterResourceHandle = resourceManager->CreateResource(bufferCreateInfo, "Clusters");
//
//	// draw box with cluster endpoints as verts for the scene
//	BufferCreateInfo clustergeobufferCI
//	{
//	.desc = StructuredBufferDesc {.data = nullptr, .elementCount = cluster_number, .elementStride = sizeof(ClusterGeometryData), .createSRV = true, .createUAV = true},
//	.usage = BufferUsage::DEFAULT,
//	.bufferResourceViewFlags = BufferViewFlags::SRV | BufferViewFlags::UAV,
//	.keepMapped = false,
//	.debugName = L"ClustersGeometry",
//	.bindlessHeap = resourceManager->GetBindlessHeap().Get() };
//
//	ResourceHandle ClusterGeometryHandle = resourceManager->CreateResource(clustergeobufferCI, "ClustersGeometry");
//
//	ShaderH clusteredComp = pipelineCache->LoadShader({
//	.shaderPath = L"../../../../shaders/Clustered/ComputeAABB.hlsl",
//	.pEntryPoint = L"ClusterMain",
//	.pTarget = L"cs_6_7",
//	.type = Type::COMPUTE });
//
//	ComputePipelineDesc computePipelineAABBdesc{.computeShader = clusteredComp };
//	ComputeAABBPipeline = pipelineCache->CreatePipeline(computePipelineAABBdesc, "ClusteredAABBCompute");
//
//	clusterSizeXYZ[0] = 16;
//	clusterSizeXYZ[1] = 9;
//	clusterSizeXYZ[2] = 24;
//
//	pushConstants.clusterInputData[0] = clusterSizeXYZ[0];
//	pushConstants.clusterInputData[1] = clusterSizeXYZ[1];
//	pushConstants.clusterInputData[2] = clusterSizeXYZ[2];
//
//	Resource* cluster = resourceManager_->GetResource(ClusterResourceHandle);
//	const u32 clusterIndex = std::get_if<Buffer>(cluster)->AsUnorderedAccessView()->heapIndex.value();
//	pushConstants.clusterUAVIndex = clusterIndex;
//
//
//	pushConstants.screenDimensions[0] = GlobalStorage::g_LumaConstants.width;
//	pushConstants.screenDimensions[1] = GlobalStorage::g_LumaConstants.height;
//}
//
//void ComputeAABBPass::Execute(RenderContext& ctx, const Scene& scene)
//{
//	auto cmdList = ctx.cmdList_;
//
//	const Camera& cam = scene.GetCamera();
//	DirectX::XMMATRIX proj = cam.projection;
//
//	// compute shader for calculating AABB
//	const Pipeline* pipeline = pipelineCache_->GetPipeline(ComputeAABBPipeline);
//
//	cmdList->SetPipelineState(pipeline->pso.Get());
//	cmdList->SetComputeRootSignature(pipeline->rootSign.Get());
//
//	pushConstants.inverseProj = DirectX::XMMatrixInverse(nullptr, proj);
//	pushConstants.zNear = cam.nearPlane;
//
//	pushConstants.zFar = cam.farPlane;
//
//	// TODO: use this instead of bindless for testing idk? A nice alternative
//	// cmdList->SetComputeRootUnorderedAccessView()
//
//	cmdList->SetComputeRoot32BitConstants(0, sizeof(ComputeAABBData) / 4,
//		&pushConstants, 0);
//
//	cmdList->Dispatch(clusterSizeXYZ[0], clusterSizeXYZ[1], clusterSizeXYZ[2]);
//}