#include "TrianglePass.h"

#include "Graphics/RHI/RHI.h"
#include "Graphics/RHI/RHITypes.h"

static float vertices[] = {
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left 
};
static unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
};


void TrianglePass::Init()
{
    BufferHandle vbHandle = RHI::CreateBuffer(
        {
        	.createInfo = RHIVertexBufferCreateInfo {.vertices = vertices, .vertexCount = _countof(vertices), .vertexStride = sizeof(vertices[0]) * 3},
            .usage = RHIMemoryeUsage::DEFAULT,
			.view = RHIResourceView::LOAD,
			.debugName = L"VertexBuffer"
        },
        nullptr);

    BufferHandle ibHandle = RHI::CreateBuffer(
        {
            .createInfo = RHIIndexBufferCreateInfo{.indices = indices, .indexCount = _countof(indices), .format = RHIFormat::R32_UINT},
            .usage = RHIMemoryeUsage::DEFAULT,
            .view = RHIResourceView::LOAD,
            .debugName = L"IndexBuffer" 
        },
        nullptr);

    ShaderHandle vertHandle = RHI::CreateShader(
        { 
        	.path = L"../../../../shaders/Triangle.hlsl",
        	.entryPoint = L"VS_Main", 
        	.target = L"vs_6_7", 
        	.stage = RHIShaderStage::VERTEX });

    ShaderHandle pixelHandle = RHI::CreateShader(
        { 
        	.path = L"../../../../shaders/Triangle.hlsl",
        	.entryPoint = L"PS_Main", 
        	.target = L"ps_6_7",
        	.stage = RHIShaderStage::PIXEL });

    RHIGraphicsPipelineDesc pipelineDesc{
        .vs = vertHandle,
        .ps = pixelHandle,
        .blend = RHIBlendMode::ADDITIVE,
        .depthFunc = RHIDepthFunc::GEQUAL,
        .rasterMode = RHIRasterMode::NONE,
        .topology = RHITopology::TRIANGLE_LIST,
        .colorFormats = {RHIFormat::R8G8B8A8_UNORM},
        .depthFormat = RHIFormat::D32_FLOAT
    };
}

void TrianglePass::Execute(RenderContext& ctx, const Scene& scene)
{

}
