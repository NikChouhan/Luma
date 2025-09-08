set_xmakever("3.0.0")
add_rules("mode.debug", "mode.release")
add_defines("UNICODE", "_UNICODE", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
set_languages("cxx23", "c17")
add_syslinks("user32.lib", "d3d12.lib", "dxgi.lib", "d3dcompiler.lib")

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
includes("scripts/packages.lua")

add_includedirs("src", "src/core", "D3D12/include")

target("Luma")
    set_kind("binary")
    set_pcxxheader("src/pch.h", {public = true})
    add_headerfiles("src/*.h", "src/core/*.h")
    add_files("src/*.cpp", "src/core/*.cpp")
    add_packages("d3d12-memory-allocator", "stb", {public = true})
target_end()