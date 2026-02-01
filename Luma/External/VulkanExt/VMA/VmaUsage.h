#if defined(RHI_BACKEND_VULKAN)

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#if defined(USE_WAYLAND)
#define VK_USE_PLATFORM_WAYLAND_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#endif

// always include volk.h before vk_mem_alloc
#include <volk/volk.h>

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <vulkan/vulkan_wayland.h>
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#include <vulkan/vulkan_xlib.h>
#endif

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#endif