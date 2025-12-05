#include "RTAO.h"

RTAOPass InitRTAOResources(const GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, DXCRes& dxcRes, Resources* resources)
{
	RTAOPass rtao{};

	ShaderDesc rtaoShaderDesc = {
	.shaderPath = L"../../../../shaders/RTAO.hlsl",
	.pEntryPoint = L"RTAOMain",
	.pTarget = L"cs_6_7",
	.type = Type::COMPUTE
	};
	rtao._rtaoShader = CreateShader(gfxDevice, resources, dxcRes, rtaoShaderDesc);

	PipelineDesc backdropComputePDesc = {
		.pipelineType = PipelineType::COMPUTE,
		.rootSignType = RootSignDesc::RSType::RTAO,
		.shaderIndex = {4},
		.enableDepthTest = FALSE,
		.enableStencilTest = FALSE,
		.isDepthPrePass = FALSE };
	Pipeline backdropComputePipeline = CreatePipeline(gfxDevice, swapchain, resources, backdropComputePDesc);


	return rtao;
}
