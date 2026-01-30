set_xmakever("3.0.0")

includes("scripts/packages.lua")
includes("Luma/xmake.lua")
includes("shaders/xmake.lua")

add_rules("mode.debug", "mode.release")
add_defines("UNICODE", "_UNICODE", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
set_languages("cxx23", "c17")
add_syslinks("user32.lib", "kernel32.lib", "shell32.lib", "comctl32.lib", "d3d12.lib", "dxgi.lib", "dxguid.lib")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

--set_config("rhi_backend", "dx12")

option("rhi_backend")
    set_default("dx12")
    set_showmenu(true)
    set_description("RHI Backend (vulkan, dx12 supported!)")
    set_values("vulkan", "dx12")
option_end()

if has_config("rhi_backend") then
    local backend = get_config("rhi_backend")
    if backend == "vulkan" then
        add_defines("RHI_BACKEND_VULKAN")
    elseif backend == "dx12" then
        add_defines("RHI_BACKEND_D3D12")
    end
end

if has_config("rhi_backend") then
    local backend = get_config("rhi_backend")
    if backend == "vulkan" then
        add_requires("spirv-reflect")
        add_requires("imgui docking", {config = { sdl2 = true, vulkan = true}})
    elseif backend == "dx12" then
        add_requires("d3d12-memory-allocator")
        add_requires("imgui docking", {config = { win32 = true, dx12 = true}})
    end
end

if (is_mode("debug")) then
    set_symbols("debug")
    add_defines("DEBUG")
    set_optimize("none")
    set_warnings("all", "extra")
    set_runtimes("MDd")
elseif(is_mode("release")) then
    add_defines("NDEBUG")
    set_optimize("fastest")
    set_strip("all")
    set_policy("build.optimization.lto", true)
    set_runtimes("MD")
end

add_includedirs("Luma", "Luma/External/D3D12Ext/include", "Luma/External/VulkanExt")

target("game")
    set_default(true)
    set_kind("binary")
    add_files("Luma/*.cpp")
    add_headerfiles("Luma/*.h")
    add_rules("c++.unity_build")
    add_deps("Luma")
target_end()