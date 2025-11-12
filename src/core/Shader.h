#pragma once
#include <dxcapi.h>
#include <d3d12shader.h>

#include "GfxDevice.h"

struct Resources;

enum class Type : u32
{
    VERTEX,
    PIXEL,
    MESH,
    COMPUTE
};

struct DXCRes
{
    ComPtr<IDxcUtils> _pUtils;
    ComPtr<IDxcCompiler3> _pCompiler;
    ComPtr<IDxcIncludeHandler> _pIncludeHandler;
};

struct Shader
{
    ComPtr<IDxcBlobEncoding> _pBlobEnc;
    ComPtr<IDxcBlob> _pBlob;
    DxcBuffer _source;
    ComPtr<IDxcResult> _result;
    Type _type{};
    u32 _index;

    void Release();
};

struct ShaderDesc
{
    const wchar_t* _shaderPath{};
    const wchar_t* _pEntryPoint;
    const wchar_t* _pTarget;
    Type _type{};
};

DXCRes ShaderCompiler();
void CompileShaderInternal(GfxDevice& gfxDevice, DXCRes& dxcRes, Shader& shader, const ShaderDesc& shaderDesc);
Shader CreateShader(GfxDevice& gfxDevice, Resources* resources, DXCRes& dxcRes, const ShaderDesc& shaderDesc);
void DestroyShader(GfxDevice& gfxDevice, DXCRes& dxcRes, Shader& shader);