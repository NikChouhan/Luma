#include "Model.h"
#include <meshoptimizer.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <stb_image.h>
#include "Core/Log.h"

static SM::Matrix NodeToMatrix(cgltf_node* node)
{
    SM::Matrix m = SM::Matrix::Identity;
    if (node->has_matrix)
    {
        memcpy(&m, node->matrix, sizeof(float) * 16);
    }
    else
    {
        SM::Vector3 t = node->has_translation ? SM::Vector3(node->translation) : SM::Vector3::Zero;
        SM::Quaternion r = node->has_rotation ? SM::Quaternion(node->rotation) : SM::Quaternion::Identity;
        SM::Vector3 s = node->has_scale ? SM::Vector3(node->scale) : SM::Vector3::One;
        m = SM::Matrix::CreateScale(s) * SM::Matrix::CreateFromQuaternion(r) * SM::Matrix::CreateTranslation(t);
    }
    return m;
}

Model::Model(const GfxDevice& gfxDevice, ResourceManager* resourceManager)
	: gfxDevice_(gfxDevice), resourceManager_(resourceManager) {}

void Model::Load(const std::string& path)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
    {
        printl(Log::LogLevel::Error, "[Model] Failed to parse: {}", path);
        return;
    }

    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        printl(Log::LogLevel::Error, "[Model] Failed to load buffers: {}", path);
        cgltf_free(data);
        return;
    }

    directory_ = path.substr(0, path.find_last_of("/\\"));

    std::vector<Vertex> allVertices;
    std::vector<u32> allIndices;

    allVertices.reserve(10000000);
    allIndices.reserve(3000000);

    cgltf_scene* scene = data->scene;
    for (size_t i = 0; i < scene->nodes_count; ++i)
    {
        ProcessNode(scene->nodes[i], SM::Matrix::Identity, allVertices, allIndices);
    }

    if (!allVertices.empty())
    {
        BufferCreateInfo vbInfo = {
            .desc = VertexBufferDesc{.vertices = allVertices.data(), .vertexCount = (u32)allVertices.size(), .vertexStride = sizeof(Vertex)},
            .usage = BufferUsage::UPLOAD,
            .debugName = L"Model_GlobalVB"
        };
        globalVertexBuffer_ = resourceManager_->CreateResource(vbInfo, path + "_VB");

        BufferCreateInfo ibInfo = {
            .desc = IndexBufferDesc{.indices = allIndices.data(), .indexCount = (u32)allIndices.size(), .indexFormat = DXGI_FORMAT_R32_UINT},
            .usage = BufferUsage::UPLOAD,
            .debugName = L"Model_GlobalIB"
        };
        globalIndexBuffer_ = resourceManager_->CreateResource(ibInfo, path + "_IB");

        printl(Log::LogLevel::Info, "[Model] Loaded {} with {} submeshes. Total Verts: {}", path, subMeshes_.size(), allVertices.size());
    }

    cgltf_free(data);
}
void Model::ProcessNode(cgltf_node* node, const SM::Matrix& parentTransform, 
    std::vector<Vertex>& allVertices, std::vector<u32>& allIndices)
{
    SM::Matrix localTransform = NodeToMatrix(node);
    SM::Matrix globalTransform = localTransform * parentTransform;

    if (node->mesh)
    {
        for (size_t i = 0; i < node->mesh->primitives_count; ++i)
        {
            ProcessPrimitive(&node->mesh->primitives[i], globalTransform, allVertices, allIndices);
        }
    }

    for (size_t i = 0; i < node->children_count; ++i)
    {
        ProcessNode(node->children[i], globalTransform, allVertices, allIndices);
    }
}

void Model::ProcessPrimitive(cgltf_primitive* primitive, const SM::Matrix& transform, std::vector<Vertex>& allVertices, std::vector<u32>& allIndices)
{
    if (primitive->type != cgltf_primitive_type_triangles) return;

    cgltf_attribute* posAttr = nullptr;
    cgltf_attribute* normAttr = nullptr;
    cgltf_attribute* texAttr = nullptr;

    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        if (strcmp(primitive->attributes[i].name, "POSITION") == 0) posAttr = &primitive->attributes[i];
        if (strcmp(primitive->attributes[i].name, "NORMAL") == 0) normAttr = &primitive->attributes[i];
        if (strcmp(primitive->attributes[i].name, "TEXCOORD_0") == 0) texAttr = &primitive->attributes[i];
    }

    if (!posAttr || !primitive->indices) return;

    size_t vertexCount = posAttr->data->count;
    size_t indexCount = primitive->indices->count;

    SubMesh subMesh;
    subMesh.baseVertex = (i32)allVertices.size();
    subMesh.startIndex = (u32)allIndices.size();
    subMesh.indexCount = (u32)indexCount;
    subMesh.transform = transform; 

    std::vector<Vertex> tempVertices(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i)
    {
        cgltf_accessor_read_float(posAttr->data, i, &tempVertices[i].position_.x, 3);

        if (texAttr) cgltf_accessor_read_float(texAttr->data, i, &tempVertices[i].texCoord_.x, 2);
        else tempVertices[i].texCoord_ = { 0, 0 };

        if (normAttr) cgltf_accessor_read_float(normAttr->data, i, &tempVertices[i].normal_.x, 3);
        else tempVertices[i].normal_ = { 0, 1, 0 };
    }

    std::vector<u32> tempIndices(indexCount);
    for (size_t i = 0; i < indexCount; ++i)
    {
        tempIndices[i] = (u32)cgltf_accessor_read_index(primitive->indices, i);
    }
    // Meshoptimizer stuff

    // Create remap table
    std::vector<u32> remap(indexCount);
    size_t uniqueVertexCount = meshopt_generateVertexRemap(remap.data(), tempIndices.data(),
        indexCount, tempVertices.data(), vertexCount, sizeof(Vertex));

    // Reindex
    std::vector<u32> optIndices(indexCount);
    std::vector<Vertex> optVertices(uniqueVertexCount);

    // Optimisation 1 - Remove duplicate vertices
    meshopt_remapIndexBuffer(optIndices.data(), tempIndices.data(), indexCount, remap.data());
    meshopt_remapVertexBuffer(optVertices.data(), tempVertices.data(), vertexCount, 
        sizeof(Vertex), remap.data());

    // Optimisation 2 - Optimize cache for locality
    meshopt_optimizeVertexCache(optIndices.data(), optIndices.data(), indexCount, uniqueVertexCount);
    // Optimisation 3 - Optimize overdraw
    meshopt_optimizeOverdraw(optIndices.data(), optIndices.data(), indexCount, &optVertices[0].position_.x, 
        uniqueVertexCount, sizeof(Vertex), 1.05f);
    // Optimization 4 - optimize access to the vertex buffer
    meshopt_optimizeVertexFetch(optVertices.data(), optIndices.data(), indexCount, 
        optVertices.data(), uniqueVertexCount, sizeof(Vertex));

    // Calculate Bounds (AABB) (will be used when I perform culling)
    SM::Vector3 min(FLT_MAX);
    SM::Vector3 max(-FLT_MAX);
    for (const auto& v : optVertices)
    {
        SM::Vector3 pos = XMLoadFloat3(&v.position_);
        min = SM::Vector3::Min(min, pos);
        max = SM::Vector3::Max(max, pos);
    }
    SM::Vector3 center = min + ((max - min) * 0.5);
    SM::Vector3 extents = (max - min) * 0.5;
    subMesh.bounds = DirectX::BoundingBox(center, extents);

    cgltf_material* material = primitive->material;
    // Material Loading
    if (material)
    {
        Material mat = {};
        if (material->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;

            if (pbr->base_color_texture.texture)
            {
                mat.baseColorFactor = SM::Vector4(pbr->base_color_factor);
                mat.albedoTexture = LoadTexture(&pbr->base_color_texture, true);
            }
            if (pbr->metallic_roughness_texture.texture)
            {
                mat.metallicFactor = pbr->metallic_factor;
                mat.roughnessFactor = pbr->roughness_factor;
                mat.metallicRoughnessTexture = LoadTexture(&pbr->metallic_roughness_texture, false);
            }
        }

        if (material->normal_texture.texture)
        {
            mat.normalTexture = LoadTexture(&primitive->material->normal_texture, false);

        }
        if (material->emissive_texture.texture)
        {
            mat.emissiveTexture = LoadTexture(&primitive->material->emissive_texture, true);

        }
        subMesh.materialIndex = (u32)materials_.size();
        materials_.push_back(mat);
    }


    allVertices.insert(allVertices.end(), optVertices.begin(), optVertices.end());
    allIndices.insert(allIndices.end(), optIndices.begin(), optIndices.end());

    subMeshes_.push_back(subMesh);
}

ResourceHandle Model::LoadTexture(const cgltf_texture_view* view, const bool sRGB) const
{
    if (!view || !view->texture || !view->texture->image || !view->texture->image->uri)
        return g_invalidResourceHandle;

    const char* uri = view->texture->image->uri;
    std::string fullPath = directory_ + "/" + uri;

    ResourceHandle handle = resourceManager_->GetResourceHandleByName(uri);
    if (resourceManager_->IsResourceHandleValid(handle))
        return handle;

    int w, h, c;
    unsigned char* data = stbi_load(fullPath.c_str(), &w, &h, &c, STBI_rgb_alpha);
    if (!data)
    {
        printl(Log::LogLevel::Warn, "[Model] Texture missing: {}", fullPath);
        return g_invalidResourceHandle;
    }

    // msft wide character bs for debug name
    std::wstring debugName(uri, uri + strlen(uri));

    TextureCreateInfo createInfo = {
        .desc = {
            .width = (u32)w,
            .height = (u32)h,
            .texPixelSize = 4,
            .format = sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM,
            .viewFlags = TextureViewFlags::SRV,
            .initialData = data
        },
        .debugName = debugName.c_str(),
        .usage = TextureUsage::UPLOAD,
        .heap = resourceManager_->GetBindlessHeap().Get()
    };

    handle = resourceManager_->CreateResource(createInfo, uri);

    stbi_image_free(data);
    return handle;
}
