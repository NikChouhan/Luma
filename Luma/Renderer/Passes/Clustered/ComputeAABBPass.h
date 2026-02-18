//#pragma once
//
//#include "Renderer/Core/RenderPass.h"
//
//struct ComputeAABBData
//{
//	DirectX::XMMATRIX inverseProj;
//
//	u32 clusterInputData[3];
//	float zNear;
//
//	u32 screenDimensions[2];
//	float zFar;
//	u32 clusterUAVIndex;
//
//	u32 clusterGeometryStructuredBufferUAVndex;
//	SM::Vector3 padding;
//};
//
//struct ClusterGeometryData
//{
//	SM::Vector4 vertices[8];
//	struct {
//		u32 indices[2];
//		u32 padding[2];
//	} triangles[18];
//};
//
//struct ComputeAABBPass : RenderPass
//{
//	void Init() override;
//	void Execute(RenderCtx& ctx, const Scene& scene) override;
//
//private:
//	PipelineHandle ComputeAABBPipeline = g_invalidPipelineHandle;
//	u32 clusterSizeXYZ[3]{ 16, 9, 24 };
//	ComputeAABBData pushConstants{};
//};