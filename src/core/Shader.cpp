#include "Shader.h"

Shader CreateShader(GfxDevice& gfxDevice, const ShaderDesc& shaderDesc)
{
    Shader shader{};

#if defined(DEBUG)
    constexpr u32 compilerFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    u32 compilerFlags = 0;
#endif
    DX_ASSERT(
            D3DCompileFromFile(shaderDesc._shaderPath, nullptr, nullptr, shaderDesc._pEntryPoint,
                shaderDesc._pTarget,compilerFlags, 0, &shader._pBlob, nullptr));

    if (shaderDesc._type == Type::VERTEX)
        shader._type = Type::VERTEX;
    else if (shaderDesc._type == Type::PIXEL)
        shader._type = Type::PIXEL;

    return shader;
}