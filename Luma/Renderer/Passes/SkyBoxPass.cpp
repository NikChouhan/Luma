#include "SkyBoxPass.h"

#include <stb_image.h>

#include "scene.h"
#include "Core/Camera.h"
#include "Renderer/Core/RenderContext.h"

void SkyBoxPass::Init()
{
	// create texture and pipeline for skybox
    {
        const char* cubemapFaces[6] = {
            "../../../../assets/skybox/starrynight/right.png",   // +X
            "../../../../assets/skybox/starrynight/left.png",    // -X
            "../../../../assets/skybox/starrynight/top.png",     // +Y
            "../../../../assets/skybox/starrynight/bottom.png",  // -Y
            "../../../../assets/skybox/starrynight/back.png",    // +Z
        	"../../../../assets/skybox/starrynight/front.png"    // -Z
        };

        int width, height, channels;
        std::vector<unsigned char*> faceData;

        for (auto& cubemapFace : cubemapFaces)
        {
            unsigned char* data = stbi_load(cubemapFace, &width, &height, &channels, STBI_rgb_alpha);
            if (!data) {
                assert(false && "Failed to load cubemap face");
            }
            faceData.push_back(data);
        }

        size_t faceSize = width * height * 4;
        size_t totalSize = faceSize * 6;

        std::vector<unsigned char> combinedData(totalSize);
        for (int i = 0; i < 6; ++i) {
            memcpy(combinedData.data() + (i * faceSize), faceData[i], faceSize);
        }

        skyboxCubemapHandle = RHI::CreateTexture(
            {
            .width = (u32)width,
            .height = u32(height),
            .depth = 0,
            .mips = 1,
            .arraySize = 6,
            .format = RHIFormat::R8G8B8A8_UNORM,
            .usage = RHIMemoryeUsage::UPLOAD,
            .view = RHIResourceView::LOAD,
            .createPerMipViews = false,
            .debugName = L"SkyboxCubeMap" }
        , combinedData.data());

        // Free stbi loaded data
        for (auto* data : faceData) {
            stbi_image_free(data);
        }
    }

    // cube geometry
    {
        struct Vertex {
            float position[3];
        };


        /*
         *      7_-----------6-_
         *      |  -_         | -_
         *      |    -3-------|---2
         *      |     |       |   |
         *      4_----|-----5_|   | 
         *         -_ |        -_ |
         *           -0-----------1
		*/

        Vertex vertices[] = {
            {{-1.0f, -1.0f, -1.0f}},
            {{ 1.0f, -1.0f, -1.0f}},
            {{ 1.0f,  1.0f, -1.0f}},
            {{-1.0f,  1.0f, -1.0f}},
            {{-1.0f, -1.0f,  1.0f}},
            {{ 1.0f, -1.0f,  1.0f}},
            {{ 1.0f,  1.0f,  1.0f}},
            {{-1.0f,  1.0f,  1.0f}}
        };


        skyboxVertexBufferHandle = RHI::CreateBuffer(
            {
                .createInfo = RHIVertexBufferCreateInfo {.vertices = vertices, .vertexCount = 8, .vertexStride = sizeof(Vertex)},
                .usage = RHIMemoryeUsage::UPLOAD,
                .view = RHIResourceView::LOAD,
                .debugName = L"SkyBoxVertexBuffer"
            },
            nullptr);


        u32 indices[] = {
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 4, 7,
            4, 0, 3, 3, 7, 4,
            1, 5, 6, 6, 2, 1,
            4, 5, 1, 1, 0, 4,
            3, 2, 6, 6, 7, 3
        };


        skyboxIndexBufferHandle = RHI::CreateBuffer(
            {
                .createInfo = RHIIndexBufferCreateInfo{.indices = indices, .indexCount = 36, .format = RHIFormat::R32_UINT},
                .usage = RHIMemoryeUsage::UPLOAD,
                .view = RHIResourceView::LOAD,
                .debugName = L"SkyBoxIndexBuffer"
            },
            nullptr);
    }
    // gfx pipeline
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements= {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        std::vector<RHIInputBindingDesc> bindingdescs =
        { {.binding = 0, .stride = sizeof(float[3]), .inputRate = RHIInputRate::PerVertex } };
        
    	std::vector<RHIInputAttributeDesc> inputAttributes = {
            {.location = 0, .binding = 0, .format = RHIFormat::R32G32B32_FLOAT, .offset = 0, .semanticName = "POSITION" },
        };

        ShaderHandle vertexShader = RHI::CreateShader({
            .path = L"../../../../shaders/Skybox.hlsl",
            .entryPoint = L"VSMain",
            .target = L"vs_6_7",
            .stage = RHIShaderStage::VERTEX
            });

        ShaderHandle pixelShader = RHI::CreateShader({
            .path = L"../../../../shaders/Skybox.hlsl",
            .entryPoint = L"PSMain",
            .target = L"ps_6_7",
            .stage = RHIShaderStage::PIXEL
            });

        skyBoxPipelineHandle = RHI::CreateGraphicsPipeline({
            .vs = vertexShader,
            .ps = pixelShader,
            .blend = RHIBlendMode::NON_TRANSPARENT,
            .depthFunc = RHIDepthFunc::GEQUAL,
            .depthMode = RHIDepthMode::READ,
            .rasterMode = RHIRasterMode::NONE,
            .topology = RHITopology::TRIANGLE_LIST,
            .colorFormats = {RHIFormat::R8G8B8A8_UNORM},
            .depthFormat = RHIFormat::D32_FLOAT,
            .inputBindings = bindingdescs,
            .inputAttributes = inputAttributes
            });
    }
}
void SkyBoxPass::Execute(RenderContext& ctx, const Scene& scene)
{
	// the usual execute pipeline stuff
    const auto cmdList = ctx.cl;


    cmdList->SetPipeline(skyBoxPipelineHandle);
    cmdList->SetViewport({});
    cmdList->SetScissor({});

    cmdList->BeginRendering({ g_invalidTextureHandle }, g_invalidTextureHandle);

    // remove translation matrix
    const Camera& cam = scene.GetCamera();
    DirectX::XMMATRIX view = cam.view;
    view.r[3] = DirectX::XMVectorSet(0, 0, 0, 1); // Zero out translation

    cmdList->BindVertexBuffer(skyboxVertexBufferHandle);
    cmdList->BindIndexBuffer(skyboxIndexBufferHandle);

    DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view,cam.projection);

    SkyBoxConstants constants{};
    constants.viewProj = viewProj;
    constants.cubemapSRVIndex = RHI::GetBindlessReadIndex(skyboxCubemapHandle);

    cmdList->SetGraphicsPushConstants(&constants, sizeof(SkyBoxConstants) / 4, 0);

    cmdList->DrawIndexed(36, 1, 0, 0, 0);

    cmdList->TextureBarrier(g_invalidTextureHandle, RHIResourceState::DEPTH_READ, RHIResourceState::DEPTH_WRITE);
}
