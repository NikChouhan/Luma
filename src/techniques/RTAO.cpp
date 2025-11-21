#include "RTAO.h"
#include "Resources.h"

RTAOPass InitRTAOResources(const GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, DXCRes& dxcRes, Resources* resources)
{
	RTAOPass rtao{};

	ShaderDesc rtaoShaderDesc = {
	._shaderPath = L"../../../../shaders/RTAO.hlsl",
	._pEntryPoint = L"RTAOMain",
	._pTarget = L"cs_6_7",
	._type = Type::COMPUTE
	};
	rtao._rtaoShader = CreateShader(gfxDevice, resources, dxcRes, rtaoShaderDesc);

	PipelineDesc backdropComputePDesc = {
		._pipelineType = PipelineType::COMPUTE,
		._rootSignType = RootSignDesc::RSType::RTAO,
		._shaderIndex = {4},
		._enableDepthTest = FALSE,
		._enableStencilTest = FALSE,
		._isDepthPrePass = FALSE };
	Pipeline backdropComputePipeline = CreatePipeline(gfxDevice, swapchain, resources, backdropComputePDesc);


	return rtao;
}
