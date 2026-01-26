target("Luma")
    set_kind("static")
    set_pcxxheader("pch.h", {public = true})
    add_headerfiles(
     "Core/*.h",
     "External/SimpleMath/SimpleMath.h",
     "External/sol/sol.hpp",
     "Graphics/*.h",
     "Graphics/RHI/D3D12/*.h",
     "Graphics/RHI/*.h",
     "Graphics/RHI/Vulkan/*.h",
     "Renderer/*.h",
     "Renderer/Core/*.h",
     "Renderer/Passes/*.h",
     "Renderer/Passes/Clustered/*.h",
     "RenderGraph/*.h"
     )

    add_files(
     "Core/*.cpp",
     "External/SimpleMath/SimpleMath.cpp",
     "Graphics/*.cpp",
     "Graphics/RHI/D3D12/*.cpp",
     "Graphics/RHI/Vulkan/*.cpp",
     "Renderer/*.cpp",
     "Renderer/Core/*cpp",
     "Renderer/Passes/*cpp",
     "Renderer/Passes/Clustered/*.cpp",
     "RenderGraph/*.cpp"
    )
    add_rules("c++.unity_build")

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