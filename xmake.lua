set_xmakever("3.0.0")

includes("scripts/packages.lua")
includes("src/xmake.lua")
includes("shaders/xmake.lua")

add_rules("mode.debug", "mode.release")
add_defines("UNICODE", "_UNICODE", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
set_languages("cxx23", "c17")
add_syslinks("user32.lib", "kernel32.lib", "shell32.lib", "comctl32.lib", "d3d12.lib", "dxgi.lib")

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

add_includedirs("src", "src/core", "D3D12/include", "src/includes")

target("game")
    set_default(true)
    set_kind("binary")
    add_files("src/*.cpp")
    add_headerfiles("src/*.h")
    add_deps("Luma")
target_end()