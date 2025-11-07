# Luma
A simple D3D12 renderer built for learning D3D12 and DXR. Atleast RTX 2000 series or AMD RDNA2+ card required for building and running. The support is tested on RTX 3060 ti (which I own)

## Features
1. GlTF model loader
2. PBR model
3. Cook Torrence BRDF
4. REBAR support (using D3D12_GPU_UPLOAD_HEAP, slightly broken rn)
5. Ray traced lighting using DXR
6. Emissive materials
7. Forward renderer with a depth pre pass
8. Shader hot reloading
9. ImGui debug menu
## WIP
1. Forward+ renderer
2. Mesh shaders
## Build
```
xmake project -k vsxmake
```
## Screenshots
![](assets/images/luma1.png?raw=true)
![](assets/images/luma2.png?raw=true)
![](assets/images/luma3.png?raw=true)