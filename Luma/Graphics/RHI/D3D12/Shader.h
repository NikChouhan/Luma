#pragma once
#include <dxcapi.h>
#include <d3d12shader.h>

#include "Graphics/GfxDevice.h"

struct Resources;

struct ShaderHandle
{
    u32 index : 24;
    u32 generation : 8;

    bool operator==(const ShaderHandle& other) const
    {
        return index == other.index && generation == other.generation;
    }
};

static constexpr ShaderHandle g_invalidShaderHandle = { .index = 0xFFFFFF, .generation = 0xFF };

enum class Type : u32
{
    VERTEX,
    PIXEL,
    MESH,
    COMPUTE
};

struct DXCRes
{
    ComPtr<IDxcUtils> pUtils;
    ComPtr<IDxcCompiler3> pCompiler;
    ComPtr<IDxcIncludeHandler> pIncludeHandler;
};

struct Shader
{
    ComPtr<IDxcBlobEncoding> pBlobEnc;
    ComPtr<IDxcBlob> pBlob;
    DxcBuffer source;
    ComPtr<IDxcResult> result;
    Type type{};
    u32 index;

    void Release();
};

struct ShaderDesc
{
    const wchar_t* shaderPath{};
    const wchar_t* pEntryPoint;
    const wchar_t* pTarget;
    Type type{};
};

DXCRes ShaderCompiler();
void CompileShaderInternal(const GfxDevice& gfxDevice, DXCRes& dxcRes, Shader& shader, const ShaderDesc& shaderDesc);
Shader CreateShader(const GfxDevice& gfxDevice, Resources* resources, DXCRes& dxcRes, const ShaderDesc& shaderDesc);
void DestroyShader(GfxDevice& gfxDevice, DXCRes& dxcRes, Shader& shader);