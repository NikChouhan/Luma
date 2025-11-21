#pragma once

#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <d3dx12/d3dx12.h>

#include <StandardTypes.h>


// function pointer thingy
#define LAMBDA(...) std::function<void(__VA_ARGS__)> const&
