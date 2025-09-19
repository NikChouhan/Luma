#pragma once

#include <unordered_map>
#include <unordered_set>

#include <cgltf.h>

#include "GfxDevice.h"
#include "Buffer.h"

struct Texture;
struct Vertex;
using namespace DirectX;
namespace SM = DirectX::SimpleMath;

struct ModelDesc
{
    std::string _path;
};

enum class TextureType
{
    ALBEDO = 1,
    NORMAL = 2,
    METALLIC_ROUGHNESS = 4,
    EMISSIVE = 8,
    SPECULAR = 16
};

struct Transformation
{
    SM::Matrix _matrix = XMMatrixIdentity();
    SM::Vector3 _position = SM::Vector3();
    SM::Vector3 _rotation = SM::Vector3();
    SM::Vector3 _scale = SM::Vector3();
};

struct MaterialConstants
{
    XMFLOAT4 _ambientColor;
    XMFLOAT4 _diffuseColor;
    XMFLOAT4 _specularColor;
    float _specularPower;
};

struct Material
{
    bool _hasAlbedo = false;
    bool _hasNormal = false;
    bool _hasMetallicRoughness = false;
    bool _hasEmissive = false;
    bool _hasAo = false;

    u32 _albedoIndex = -1;
    u32 _normalIndex = -1;
    u32 _emmisiveIndex = -1;
    u32 _metallicIndex = -1;

    std::string _albedoPath;
    std::string _normalPath;
    std::string _metallicRoughnessPath;
    std::string _emissivePath;
    std::string _aoPath;

    // no texture views unlike in vulkan
    // cuz the views are created with
    // the texture heaps and aren't separate.
    // can be accessed with cpudescriptorhandle

    DirectX::XMFLOAT3 _flatColor;
};

struct Mesh
{
    std::vector<Vertex> _vertices;
    std::vector<u32> _indices;
    u32 _vertexCount;
    u32 _indexCount;
};

struct MeshInfo
{
    size_t _vertexCount = 0;
    size_t _indexCount = 0;
    u32 _materialIndex = -1;
    uint32_t _startIndex = 0;
    uint32_t _startVertex = 0;
    Transformation _transform{};
    SM::Matrix _normalMatrix{};
};

struct Model
{
    std::string _dirPath{};
    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{};
    std::vector< MeshInfo> _meshes{};
    std::vector<Material> _materials{};

    Buffer _vertexBuffer;
    Buffer _indexBuffer;

    std::vector<Texture> _modelTextures;
    ComPtr<ID3D12DescriptorHeap> _modelHeap;

    std::unordered_set<std::string> _loadedTextures; // To track loaded textures
    std::unordered_map<cgltf_material*, size_t> _materialLookup;
    std::unordered_map<std::string, size_t> _textureIndexLookup;

    auto begin() { return _meshes.begin(); }
    auto end() { return _meshes.end(); }
};

Model LoadModel(GfxDevice& gfxDevice, FrameSync& frameSync, ModelDesc desc);
void DestroyModel(GfxDevice& gfxDevice, Model& model);
