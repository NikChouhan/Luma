target("Luma")
    set_kind("static")
    set_pcxxheader("pch.h", {public = true})
    add_headerfiles(
     "Core/*.h",
     "External/SimpleMath/SimpleMath.h",
     "External/sol/sol.hpp",
     "Graphics/*.h",
     "Graphics/RHI/*.h",
     "Renderer/*.h",
     "Renderer/Core/*.h",
     "Renderer/Passes/*.h",
     "Renderer/Passes/Clustered/*.h",
     "RenderGraph/*.h", {public = true})
     
    add_files(
     "Core/*.cpp",
     "External/SimpleMath/SimpleMath.cpp",
     "Renderer/*.cpp",
     "Renderer/Core/*cpp",
     "Renderer/Passes/*cpp",
     "Renderer/Passes/Clustered/*.cpp",
     "RenderGraph/*.cpp"
    )
    add_rules("c++.unity_build")

    if has_config("rhi_backend") then
        local backend = get_config("rhi_backend")
        if backend == "dx12" then
            add_headerfiles("Graphics/RHI/D3D12/*.h")
            add_packages("d3d12-memory-allocator",
            "directxshadercompiler", {public = true})
        
            add_files("Graphics/RHI/D3D12/*.cpp")
        elseif backend == "vulkan" then
            add_headerfiles("Graphics/RHI/Vulkan/*.h")
            add_packages("spirv-reflect") -- so i am thinking if i should wtite slang or hlsl??)
            add_files("Graphics/RHI/Vulkan/*.cpp", "External/VulkanExt/vk-bootstrap/*.cpp", "External/VulkanExt/VMA/*.cpp")
        end
    end
    add_packages(
     "stb",
     "cgltf",
     "meshoptimizer",
     "imgui",
     "luajit",
     "directxmath",
     "libsdl2",
     {public = true}
    )
target_end()