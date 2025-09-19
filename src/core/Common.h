#pragma once

#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>
#include "SimpleMath.h"
#include <d3dx12/d3dx12.h>

#include "Log.h"
using namespace Luma;
#include <StandardTypes.h>

namespace SM = DirectX::SimpleMath;

constexpr u32 frameCount = 2;
#define MAX_TEXTURES 1024

#define DX_ASSERT(call)                                                                                     \
    do                                                                                                      \
    {                                                                                                       \
        HRESULT result = call;                                                                           \
        if (result != S_OK)                                                                 \
        {                                                                                                   \
            fprintf(stderr, "D3D12 error %d at %s:%d\n", static_cast<int>(result), __FILE__, __LINE__);    \
			abort();                                                                                        \
        }                                                                                                   \
    } while (0)

// function pointer thingy
#define LAMBDA(...) std::function<void(__VA_ARGS__)> const&