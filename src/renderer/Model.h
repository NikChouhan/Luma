#pragma once

#include <unordered_map>
#include <unordered_set>

#include <cgltf.h>

#include "GfxDevice.h"
#include "Buffer.h"

struct Swapchain;
struct Texture;
struct Vertex;
struct Buffer;
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

struct Material
{
    XMFLOAT4 _globalAmbientColor;

    float _opacity;
    float _specularPower;
    float _indexOfRefraction;
    BOOL  _hasAmbientTexture = FALSE;

    BOOL _hasEmissive = FALSE;
    BOOL _hasAlbedo = FALSE;
    BOOL _hasSpecular = FALSE;
    BOOL _hasSpecularPower = FALSE;

    BOOL _hasNormal = FALSE;
    BOOL _hasBump = FALSE;
    BOOL _hasOpacity = FALSE;
    float _bumpIntensity;

    u32 _albedoIndex = -1;
    u32 _normalIndex = -1;
    u32 _emmisiveIndex = -1;
    u32 _metallicRoughnessIndex = -1;

    u32 _roughnessIndex = -1;
    BOOL _hasAo = FALSE;
    float _specularScale;
    float _alphaThreshold;

    // no texture views unlike in vulkan
    // cuz the views are created with
    // the texture heaps and aren't separate.
    // can be accessed with cpudescriptorhandle
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
    ComPtr<ID3D12DescriptorHeap> _commonHeap;
    ComPtr<ID3D12DescriptorHeap> _samplerHeap;
    
    // acceleration structures
    ComPtr<ID3D12Resource> _bottomLevelAccelerationStructure;
    ComPtr<ID3D12Resource> _topLevelAccelerationStructure;
    // normal and rt uav
    ComPtr<ID3D12Resource> _normalUAV;
    ComPtr<ID3D12Resource> _rtaoUAV;

    std::unordered_set<std::string> _loadedTextures; // To track loaded textures
    std::unordered_map<cgltf_material*, size_t> _materialLookup;
    std::unordered_map<std::string, size_t> _textureIndexLookup;

    auto begin() { return _meshes.begin(); }
    auto end() { return _meshes.end(); }
};

Model LoadModel(GfxDevice& gfxDevice, FrameSync& frameSync, Swapchain& swapchain, ID3D12GraphicsCommandList10 *commandList, ModelDesc desc);
void DestroyModel(GfxDevice& gfxDevice, Model& model);
