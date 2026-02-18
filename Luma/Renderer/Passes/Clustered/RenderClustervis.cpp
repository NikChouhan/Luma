//#include "RenderClustervis.h"
//
//#include "scene.h"
//#include "Core/Camera.h"
//#include "Renderer/Core/RenderCtx.h"
//
//void RenderClusterVis::Init()
//{
//	ShaderH vertShader = pipelineCache->LoadShader({
//		.shaderPath = L"../../../../shaders/Clustered/ClusterRender.hlsl",
//		.pEntryPoint = L"VSMain",
//		.pTarget = L"vs_6_7",
//		.type = Type::VERTEX });
//
//	ShaderH pixelShader = pipelineCache->LoadShader({
//	.shaderPath = L"../../../../shaders/Clustered/ClusterRender.hlsl",
//	.pEntryPoint = L"PSMain",
//	.pTarget = L"ps_6_7",
//	.type = Type::PIXEL });
//
//	GraphicsPipelineDesc desc
//	{
//		.vertexShader = vertShader,
//		.pixelShader = pixelShader,
//		.blendMode = BlendMode::NON_TRANSPARENT,
//		.depthMode = DepthMode::READ_WRITE,
//		.depthFunc = DepthFunc::GREATER,
//		.rasterMode = RasterMode::WIREFRAME,
//		.topology = Topology::TRIANGLES,
//	};
//	ClusterRenderPipeline = pipelineCache->CreatePipeline(desc, "ClusterRenderPipeline");
//
//	ResourceHandle ClusterGeometryHandle = resourceManager->GetResourceHandleByName("ClustersGeometry");
//	Resource* clusterGeometry = resourceManager->GetResource(ClusterGeometryHandle);
//	const u32 clusterGeometrySRVIndex = std::get_if<Buffer>(clusterGeometry)->AsShaderResourceView()->heapIndex.value();
//	pushConstants.clusterGeometryStructuredBufferSRVIndex = clusterGeometrySRVIndex;
//}
//
//void RenderClusterVis::Execute(RenderCtx& ctx, const Scene& scene)
//{
//	auto cmdList = ctx.cmdList_;
//
//	const Camera& cam = scene.GetCamera();
//	DirectX::XMMATRIX proj = cam.projection;
//	const Pipeline* pipeline = pipelineCache_->GetPipeline(ClusterRenderPipeline);
//
//	pipeline->Bind(cmdList);
//	cmdList->SetGraphicsRootSignature(pipeline->rootSign.Get());
//	pushConstants.viewProj = proj;
//
//	cmdList->SetGraphicsRoot32BitConstants(0, sizeof(ClusterRender) / 4, &pushConstants, 0);
//
//	cmdList->DrawInstanced(36, NumClusters, 0, 0);
//
//	// very big issue, currently this writes over the depth buffer making it useless
//	// i need to resolve that along with having implicit conversion of resources if the current pass needs it in that state
//	// wont be hard, write a function in render pass that runs after every pass Execute, take in deps of the next pass (this will be hard)
//	// and then convert it to required ones
//}
