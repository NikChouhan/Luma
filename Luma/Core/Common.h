#pragma once

//#if defined(RHI_BACKEND_VULKAN)
//
//#ifdef _WIN32
//#define VK_USE_PLATFORM_WIN32_KHR
//#define GLFW_EXPOSE_NATIVE_WIN32
//#elif defined(__linux__)
//#if defined(USE_WAYLAND)
//#define VK_USE_PLATFORM_WAYLAND_KHR
//#define GLFW_EXPOSE_NATIVE_WAYLAND
//#else
//#define VK_USE_PLATFORM_XLIB_KHR
//#define GLFW_EXPOSE_NATIVE_X11
//#endif
//#endif
//
//#define VK_NO_PROTOTYPES
//#include <vulkan/vulkan.h>
//
//#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
//#include <vulkan/vulkan_wayland.h>
//#elif defined(VK_USE_PLATFORM_WIN32_KHR)
//#include <Windows.h>
//#include <vulkan/vulkan_win32.h>
//#elif defined(VK_USE_PLATFORM_XLIB_KHR)
//#include <vulkan/vulkan_xlib.h>
//#endif
//
//#endif

#if defined(RHI_BACKEND_D3D12)
#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#elif RHI_BACKEND_VULKAN
// include vulkan stuff if needed
#endif

#include <DirectXMath.h>
#include "External/SimpleMath/SimpleMath.h"
//#include <d3dx12/d3dx12.h>

#include "Core/Log.h"
using namespace Luma;
#include <StandardTypes.h>

namespace SM = DirectX::SimpleMath;

u32 constexpr NumClusters = 16 * 9 * 24;
u32 constexpr max_lights = 1024;
u32 constexpr maxLightIndices = NumClusters * max_lights;

constexpr u32 frameCount = 2;
#define MAX_TEXTURES 1024
#define REBAR 0

#if defined(RHI_BACKEND_D3D12)
#define RHI_ASSERT(call)                                                                                     \
    do                                                                                                      \
    {                                                                                                       \
        HRESULT result = call;                                                                              \
        if (result != S_OK)                                                                                 \
        {                                                                                                   \
            char buffer[512];                                                                               \
            snprintf(buffer, sizeof(buffer), "D3D12 error 0x%08X at %s:%d",                                 \
                     static_cast<int>(result), __FILE__, __LINE__);                                         \
            MessageBoxA(NULL, buffer, "DirectX Error", MB_OK | MB_ICONERROR);                               \
            abort();                                                                                        \
        }                                                                                                   \
    } while (0)                  \


#elif(RHI_BACKEND_VULKAN)
#define RHI_ASSERT(call)                                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        VkResult result = call;                                                                           \
        if (result != VK_SUCCESS)                                                                 \
        {                                                                                                   \
            fprintf(stderr, "Vulkan error %d at %s:%d\n", static_cast<int>(result), __FILE__, __LINE__);    \
			abort();                                                                                        \
        }                                                                                                   \
    } while (0)

#endif
// function pointer thingy
#define LAMBDA(...) std::function<void(__VA_ARGS__)> const&

//#define ARRAY_SIZE(var) {sizeof(var) / sizeof(var[0]) }
