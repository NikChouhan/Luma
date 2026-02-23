# Luma
A simple cross platform (Linux/Windows) renderer built for learning modern graphical techniques.

Linux + Vulkan support WIP. Already implemented the base RHI [here](https://github.com/NikChouhan/Luma/blob/main/Luma/Graphics/RHI/RHI.h). D3D12 backend works flawlessly

## Features
1. GlTF model loader
2. PBR model
3. Cook Torrence BRDF
4. REBAR support (using D3D12_GPU_UPLOAD_HEAP, slightly broken rn)
5. Ray traced lighting using DXR (RT branch)
6. Emissive materials
7. Forward renderer with a depth pre pass
8. Shader hot reloading (RT branch)
9. ImGui debug menu
## WIP
1. Vulkan Backend
2. Mesh shaders
3. Lua based Render graph
## Build
```
xmake project -k vsxmake
```
## Screenshots (pertaining RT branch)
![](assets/images/luma1.png?raw=true)
![](assets/images/luma2.png?raw=true)
![](assets/images/luma3.png?raw=true)
