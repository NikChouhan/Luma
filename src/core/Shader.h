#pragma once
#include "GfxDevice.h"

enum class Type : u32
{
    VERTEX,
    PIXEL,
    MESH
};

struct Shader
{
    ComPtr<ID3DBlob> _pBlob;
    Type _type{};
};

typedef std::initializer_list<Shader> Shaders;

struct ShaderDesc
{
    const wchar_t* _shaderPath;
    const char* _pEntryPoint;
    const char* _pTarget;
    Type _type{};
};

Shader CreateShader(GfxDevice& gfxDevice, const ShaderDesc& shaderDesc);
void DestroyShader(GfxDevice& gfxDevice, Shader& shader);