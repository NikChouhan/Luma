#pragma once
#include <vector>
#include <string>
#include <External/SimpleMath/SimpleMath.h>
#include "Core/Resources.h"

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

    ResourceHandle albedoTexture = g_invalidResourceHandle;
    ResourceHandle normalTexture = g_invalidResourceHandle;
    ResourceHandle metallicRoughnessTexture = g_invalidResourceHandle;
    ResourceHandle emissiveTexture = g_invalidResourceHandle;
};

class Model
{
public:
    Model(const GfxDevice& gfxDevice, ResourceManager* resourceManager);
    ~Model() = default;

    void Load(const std::string& path);

    [[nodiscard]] const std::vector<SubMesh>& GetSubMeshes() const { return subMeshes_; }
    [[nodiscard]] const std::vector<Material>& GetMaterials() const { return materials_; }

    ResourceHandle GetVertexBuffer() const { return globalVertexBuffer_; }
    ResourceHandle GetIndexBuffer() const { return globalIndexBuffer_; }

private:
    const GfxDevice& gfxDevice_;
    ResourceManager* resourceManager_;
    std::string directory_;

	ResourceHandle globalVertexBuffer_ = g_invalidResourceHandle;
    ResourceHandle globalIndexBuffer_ = g_invalidResourceHandle;

    std::vector<SubMesh> subMeshes_;
    std::vector<Material> materials_;

    void ProcessNode(cgltf_node* node, const SM::Matrix& parentTransform, std::vector<Vertex>& allVertices, std::vector<u32>& allIndices);
    void ProcessPrimitive(cgltf_primitive* primitive, const SM::Matrix& transform, std::vector<Vertex>& allVertices, std::vector<u32>& allIndices);
    ResourceHandle LoadTexture(const cgltf_texture_view* view, bool sRGB) const;
};