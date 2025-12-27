#include "SkyBoxPass.h"

#include <stb_image.h>

#include "scene.h"
#include "Core/Camera.h"
#include "Renderer/Core/PipelineCache.h"
#include "Renderer/Core/RenderContext.h"

void SkyBoxPass::Init(ResourceManager* resourceManager, PipelineCache* pipelineCache)
{
    resourceManager_ = resourceManager;
    pipelineCache_ = pipelineCache;
	// create texture and pipeline for skybox
    {
        const char* cubemapFaces[6] = {
            "../../../../assets/skybox/right.png",   // +X
            "../../../../assets/skybox/left.png",    // -X
            "../../../../assets/skybox/top.png",     // +Y
            "../../../../assets/skybox/bottom.png",  // -Y
            "../../../../assets/skybox/back.png",    // +Z
        	"../../../../assets/skybox/front.png"    // -Z
        };

        int width, height, channels;
        std::vector<unsigned char*> faceData;

        for (auto& cubemapFace : cubemapFaces)
        {
            unsigned char* data = stbi_load(cubemapFace, &width, &height, &channels, 4); // Force RGBA
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

        TextureCreateInfo cubemapCreateInfo{
            .desc = {
                .width = static_cast<u32>(width),
                .height = static_cast<u32>(height),
                .depth = 1,
                .texPixelSize = 4,
                .mipLevels = 1,
                .arraySize = 6,  // 6 faces for cubemap
                .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .viewFlags = TextureViewFlags::SRV,
                .createMipUAVs = false,
                .initialData = combinedData.data()
            },
            .debugName = L"SkyboxCubMap",
            .usage = TextureUsage::DEFAULT,
            .heap = resourceManager->GetBindlessHeap().Get()
        };

        skyboxCubemapHandle = resourceManager->CreateResource(cubemapCreateInfo, "SkyboxCubeMap");

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

        BufferCreateInfo vertexBufferInfo{
            .desc = VertexBufferDesc{
                .vertices = vertices,
                .vertexCount = 8,
                .vertexStride = sizeof(Vertex)
            },
            .usage = BufferUsage::UPLOAD,
            .debugName = L"SkyboxVertexBuffer",
        };

        skyboxVertexBufferHandle = resourceManager->CreateResource(vertexBufferInfo, "SkyboxVertexBuffer");

        u32 indices[] = {
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 4, 7,
            4, 0, 3, 3, 7, 4,
            1, 5, 6, 6, 2, 1,
            4, 5, 1, 1, 0, 4,
            3, 2, 6, 6, 7, 3
        };

        BufferCreateInfo indexBufferInfo{
            .desc = IndexBufferDesc{
                .indices = indices,
                .indexCount = 36,
                .indexFormat = DXGI_FORMAT_R32_UINT
            },
            .usage = BufferUsage::UPLOAD,
            .debugName = L"SkyboxIndexBuffer",
        };

        skyboxIndexBufferHandle = resourceManager->CreateResource(indexBufferInfo, "SkyboxIndexBuffer");
    }

    // Create graphics pipeline
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements= {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        ShaderHandle vertexShader = pipelineCache_->LoadShader({
            .shaderPath = L"../../../../shaders/Skybox.hlsl",
            .pEntryPoint = L"VSMain",
            .pTarget = L"vs_6_7",
            .type = Type::VERTEX
            });

        ShaderHandle pixelShader = pipelineCache->LoadShader({
            .shaderPath = L"../../../../shaders/Skybox.hlsl",
            .pEntryPoint = L"PSMain",
            .pTarget = L"ps_6_7",
            .type = Type::PIXEL
            });
        GraphicsPipelineDesc skyboxPipelineDesc
        {
            .vertexShader = vertexShader,
            .pixelShader = pixelShader,
            .blendMode = BlendMode::NON_TRANSPARENT,
            .depthMode = DepthMode::READ_ONLY,
            .depthFunc = DepthFunc::LESS_EQUAL,
            .rasterMode = RasterMode::SOLID_NONE_CULL,
            .topology = Topology::TRIANGLES,
            .rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
            .dsvFormat = DXGI_FORMAT_D32_FLOAT,
            .inputLayout = inputElements
        };
        SkyBoxPipelineHandle = pipelineCache->CreatePipeline(skyboxPipelineDesc, "SkyBoxPipeline");
    }
}
void SkyBoxPass::Execute(RenderContext& ctx, const Scene& scene)
{
	// the usual execute pipeline stuff
    const auto cmdList = ctx.cmdList_;

    Resource* cubemap = resourceManager_->GetResource(skyboxCubemapHandle);

    Resource* vbRes = resourceManager_->GetResource(skyboxVertexBufferHandle);
    Resource* ibRes = resourceManager_->GetResource(skyboxIndexBufferHandle);

    Pipeline* pipeline = pipelineCache_->GetPipeline(SkyBoxPipelineHandle);

    cmdList->SetPipelineState(pipeline->pso.Get());
    cmdList->SetGraphicsRootSignature(pipeline->rootSign.Get());

    cmdList->RSSetViewports(1, &ctx.viewport);
    cmdList->RSSetScissorRects(1, &ctx.scissorRect);

    cmdList->OMSetRenderTargets(1, &ctx.currentRtv,
        FALSE, &ctx.currentDsv);

    // remove translation matrix
    const Camera& cam = scene.GetCamera();
    DirectX::XMMATRIX view = cam._view;
    view.r[3] = DirectX::XMVectorSet(0, 0, 0, 1); // Zero out translation

    DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view,cam._projection);

    SkyBoxConstants constants{};
    constants.viewProj = viewProj;
    constants.cubemapSRVIndex = std::get_if<Texture>(cubemap)->srvIndex.value();

    cmdList->SetGraphicsRoot32BitConstants(0, sizeof(SkyBoxConstants) / 4, &constants, 0);

    const VertexBufferView* vbView = std::get_if<Buffer>(vbRes)->AsVertexBuffer();
    const IndexBufferView* ibView = std::get_if<Buffer>(ibRes)->AsIndexBuffer();

    cmdList->IASetVertexBuffers(0, 1, &vbView->view);
    cmdList->IASetIndexBuffer(&ibView->view);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
