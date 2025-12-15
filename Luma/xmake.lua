target("Luma")
    set_kind("static")
    set_pcxxheader("pch.h", {public = true})
    add_headerfiles(
     "Core/*.h",
     "External/SimpleMath/SimpleMath.h",
     "External/sol/sol.hpp",
     "Graphics/*.h",
     "Graphics/D3D12/*.h",
     "Renderer/*.h",
     "Renderer/Core/*.h",
     "Renderer/Passes/*.h",
     "RenderGraph/*.h"
     )

    add_files(
     "Core/*.cpp",
     "External/SimpleMath/SimpleMath.cpp",
     "Graphics/*.cpp",
     "Graphics/D3D12/*.cpp",
     "Renderer/*.cpp",
     "Renderer/Core/*cpp",
     "Renderer/Passes/*cpp",
     "RenderGraph/*.cpp"
    )

    add_packages("d3d12-memory-allocator",
     "stb",
     "cgltf",
     "directxshadercompiler",
     "meshoptimizer",
     "imgui",
     "luajit",
     {public = true}
    )
target_end()