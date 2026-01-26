#pragma once
#include <vector>
#include <string>
#include <External/SimpleMath/SimpleMath.h>
#include "Graphics/RHI/RHITypes.h"

struct cgltf_node;
struct cgltf_primitive;
struct cgltf_texture_view;


struct SubMesh
{
    u32 indexCount = 0;
    u32 startIndex = 0;
    i32 baseVertex = 0;
    u32 materialIndex = 0;

    SM::Matrix transform = SM::Matrix::Identity; // local transform from glTF node
    DirectX::BoundingBox bounds;         // for frustum culling
};

struct Material
{
    SM::Vector4 baseColorFactor = SM::Vector4::One;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float emissiveFactor = 1.0f;

    TextureHandle albedoTexture = g_invalidTextureHandle;
    TextureHandle normalTexture = g_invalidTextureHandle;
    TextureHandle metallicRoughnessTexture = g_invalidTextureHandle;
    TextureHandle emissiveTexture = g_invalidTextureHandle;
};

class Model
{
public:
    Model() = default;
    ~Model() = default;

    void Load(const std::string& path);

    [[nodiscard]] const std::vector<SubMesh>& GetSubMeshes() const { return subMeshes; }
    [[nodiscard]] const std::vector<Material>& GetMaterials() const { return materials; }

    BufferHandle GetVertexBuffer() const { return globalVertexBuffer; }
    BufferHandle GetIndexBuffer() const { return globalIndexBuffer; }

private:
    std::string directory_;

	BufferHandle globalVertexBuffer = g_invalidBufferHandle;
    BufferHandle globalIndexBuffer = g_invalidBufferHandle;

    std::vector<SubMesh> subMeshes;
    std::vector<Material> materials;

    std::unordered_map<uintptr_t, TextureHandle> textureCache;

    void ProcessNode(cgltf_node* node, const SM::Matrix& parentTransform, std::vector<Vertex>& allVertices, std::vector<u32>& allIndices);
    void ProcessPrimitive(cgltf_primitive* primitive, const SM::Matrix& transform, std::vector<Vertex>& allVertices, std::vector<u32>& allIndices);
    TextureHandle LoadTexture(const cgltf_texture_view* view, bool sRGB);
};