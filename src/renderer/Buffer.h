#pragma once

#include <imgui_internal.h>

#include "Common.h"
#include "GfxDevice.h"

typedef u32 Index;
using namespace DirectX;

namespace D3D12MA
{
	class Allocation;
}


enum class BufferType
{
	VERTEX,
    INDEX,
    UAV
};
struct HeapTextureIndex
{
    u32 _accelerationStructureIndex{};
    u32 _computeShaderBgIndex{};
    u32 _depthSRV{};
    u32 _normalUAV{};
    u32 _rtaoUAV{};
};
struct PipelineIndex
{
    u8 BGComputePass{};
    u8 DepthPrePass{};
    u8 RTAOPass{};
    u8 ImguiPass{};
    u8 RenderPass{};
};
struct LightSettings
{
    float pointIntensity = 70.0f;
    float pointColor[3] = { 0.8f, 0.6f, 0.5f };
    float pointRadius = 10.0f;
    float dirIntensity = 2.0f;
    float dirColor[3] = { 1.0f, 0.85f, 0.6f };
    SM::Vector3 direction = { -1.0f, -2.0f, 0.0f };
};

namespace GlobalStorage
{
    inline HeapTextureIndex index{};
    inline PipelineIndex pipelineIndex{};
    inline XMMATRIX _projMatrixInv{};
    inline XMMATRIX _viewMatrixInv{};
    inline LightSettings g_lightSettings;
}

inline void SetupLightSettingsHandler()
{
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "LightSettings";
    ini_handler.TypeHash = ImHashStr("LightSettings");

    ini_handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) { return (void*)1; };
    ini_handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
        float x, y, z;
        if (sscanf(line, "pointIntensity=%f", &x) == 1)
	        GlobalStorage::g_lightSettings.pointIntensity = x;
        else if (sscanf(line, "pointColor=%f,%f,%f", &x, &y, &z) == 3) {
	        GlobalStorage::g_lightSettings.pointColor[0] = x;
	        GlobalStorage::g_lightSettings.pointColor[1] = y;
	        GlobalStorage::g_lightSettings.pointColor[2] = z;
        }
        else if (sscanf(line, "pointRadius=%f", &x) == 1)
	        GlobalStorage::g_lightSettings.pointRadius = x;
        else if (sscanf(line, "dirIntensity=%f", &x) == 1)
	        GlobalStorage::g_lightSettings.dirIntensity = x;
        else if (sscanf(line, "dirColor=%f,%f,%f", &x, &y, &z) == 3) {
	        GlobalStorage::g_lightSettings.dirColor[0] = x;
	        GlobalStorage::g_lightSettings.dirColor[1] = y;
	        GlobalStorage::g_lightSettings.dirColor[2] = z;
        }
        else if (sscanf(line, "direction=%f,%f,%f", &x, &y, &z) == 3)
	        GlobalStorage::g_lightSettings.direction = SM::Vector3(x, y, z);
        };

    ini_handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        buf->appendf("[%s][Settings]\n", handler->TypeName);
        buf->appendf("pointIntensity=%.3f\n", GlobalStorage::g_lightSettings.pointIntensity);
        buf->appendf("pointColor=%.3f,%.3f,%.3f\n",
            GlobalStorage::g_lightSettings.pointColor[0], GlobalStorage::g_lightSettings.pointColor[1], GlobalStorage::g_lightSettings.pointColor[2]);
        buf->appendf("pointRadius=%.3f\n", GlobalStorage::g_lightSettings.pointRadius);
        buf->appendf("dirIntensity=%.3f\n", GlobalStorage::g_lightSettings.dirIntensity);
        buf->appendf("dirColor=%.3f,%.3f,%.3f\n",
            GlobalStorage::g_lightSettings.dirColor[0], GlobalStorage::g_lightSettings.dirColor[1], GlobalStorage::g_lightSettings.dirColor[2]);
        buf->appendf("direction=%.3f,%.3f,%.3f\n",
            GlobalStorage::g_lightSettings.direction.x, GlobalStorage::g_lightSettings.direction.y, GlobalStorage::g_lightSettings.direction.z);
        buf->append("\n");
        };

    ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
}

struct RenderPass
{
    XMMATRIX _worldViewProj;

    XMMATRIX _worldMatrix;

    u32 _albedoIndex;
    u32 _normalIndex;
    u32 _metallicRoughnessIndex;
    u32 _emissiveIndex;

    u32 _accelerationStructureIndex;
    SM::Vector3 _dirLightDir;

    float _dirLightIntensity;
    float _dirLightColor[3];

    float _pointLightIntensity;
    SM::Vector3 _cameraPos;

    float _pointLightRadius;
    float _pointLightColor[3];
};

struct DepthPPBuffer
{
    XMMATRIX _worldViewProj;
    XMMATRIX _worldMatrix;
};

struct ShaderEffects
{
    float _resolution[2];
    float _time;
    float _cameraYaw;

    float _cameraPitch;
    u32 _uavIndex;
    u32 _padding1;
    u32 _padding2;

    alignas(16) SM::Vector3 _cameraPos;
};

struct RTAO
{
    XMMATRIX _projMatrixInv;
	XMMATRIX viewMatrixInv;

    u32 _accelerationStructureIndex;
    u32 _rtUAVIndex;
    u32 _depthIndex;
    u32 _normalUAVIndex;

    BOOL _isEnabled;
    u32 _samplesPerPixel;
    u32 padding;
    u32 padding1;
};

struct Vertex
{
    XMFLOAT3 _position;
    XMFLOAT2 _texCoord;
    XMFLOAT3 _normal;
};

struct Buffer
{
    ComPtr<ID3D12Resource> _resource;
    D3D12MA::Allocation* _allocation;
    union
	{
        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
        D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    };
};

struct BufferDesc
{
    u32 _bufferSize = 0;
    /* buffer may or may not have contents. Imagine just a pointer to memory
       in a shader reflection system where a change in GPU code affects CPU side memory
    */
    BufferType _bufferType;
    void* _pContents = nullptr; 
};

Buffer CreateBuffer(GfxDevice& gfxDevice, BufferDesc desc);
void DestroyBuffer(GfxDevice& gfxDevice, Buffer& buffer);

inline void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX_ASSERT(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initialResourceState,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
}

inline void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
    DX_ASSERT(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
    void* pMappedData;
    (*ppResource)->Map(0, nullptr, &pMappedData);
    memcpy(pMappedData, pData, datasize);
    (*ppResource)->Unmap(0, nullptr);
}