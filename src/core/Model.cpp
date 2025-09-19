#include "Model.h"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <stb_image.h>
#include "Buffer.h"

#include "FrameSync.h"
#include "Texture.h"

#include "Log.h"


static u32 LoadMaterialTexture(GfxDevice& gfxDevice, FrameSync& frameSync, Model& model,
    Material& mat, const cgltf_texture_view* textureView, TextureType type)
{
    if (textureView && textureView->texture && textureView->texture->image)
    {
        cgltf_image* image = textureView->texture->image;
        std::string path = model._dirPath + "/" + std::string(image->uri);

        int width, height, channels;
        unsigned char* imgData = stbi_load(path.c_str(), &width, &height,
            &channels, STBI_rgb_alpha);

        Texture texture = CreateTexture(gfxDevice, frameSync,
            {
            ._texWidth = u32(width),
            ._texHeight = u32(height),
            ._texPixelSize = u32(channels),
            ._pContents = imgData
            });
        // Push the texture and return its new index
        model._modelTextures.push_back(texture);
        u32 newIndex = model._modelTextures.size() - 1;

        // in d3d12 heap is used to create the heap, and handled by the texture class,
        // and not model

        //if (type == TextureType::ALBEDO) mat.AlbedoView = tex._imageView;
        //if (type == TextureType::NORMAL) mat.NormalView = tex._imageView;
        //if (type == TextureType::METALLIC_ROUGHNESS) mat.MetallicRoughnessView = tex._imageView;
        //if (type == TextureType::EMISSIVE) mat.EmissiveView = tex._imageView;

        return newIndex;

        // case TextureType::AO:
        //     mat.AOView = tex.m_texImageView;
        //     mat.HasAO = true;
        //     mat.AOPath = path;
        //     modelTextures.push_back(tex);

    }
    // Handle missing texture or image
    printl(Log::LogLevel::Warn, "[Texture] Texture or image not found for material : {}", std::to_string(static_cast<int>(type)));
    return 1;
}

static void OptimiseMesh(const MeshInfo& meshInfo, const Mesh& mesh)
{
	
}

static void ProcessPrimitive(GfxDevice& gfxDevice, FrameSync& frameSync, 
    cgltf_primitive* primitive, Model& model, Transformation& parentTransform)
{
    u32 vertexOffset = model._vertices.size();
    u32 indexOffset = model._indices.size();

    std::vector<Vertex> tempVertices;
    std::vector<u32> tempIndices;

    if (primitive->type != cgltf_primitive_type_triangles)
    {
        printl(Log::LogLevel::Error, "[CGLTF] Primitive type is not triangles");
        return;
    }

    if (primitive->indices == nullptr)
    {
        printl(Log::LogLevel::Error, "[CGLTF] Primitive has no indices");
        return;
    }

    if (primitive->material == nullptr)
    {
        printl(Log::LogLevel::Error, "[CGLTF] Primitive has no material");
        return;
    }

    MeshInfo meshInfo;

    meshInfo._transform = parentTransform;

    // Get attributes
    cgltf_attribute* pos_attribute = nullptr;
    cgltf_attribute* tex_attribute = nullptr;
    cgltf_attribute* norm_attribute = nullptr;

    for (size_t i = 0; i < primitive->attributes_count; i++)
    {
        if (strcmp(primitive->attributes[i].name, "POSITION") == 0)
        {
            pos_attribute = &primitive->attributes[i];
        }
        if (strcmp(primitive->attributes[i].name, "TEXCOORD_0") == 0)
        {
            tex_attribute = &primitive->attributes[i];
        }
        if (strcmp(primitive->attributes[i].name, "NORMAL") == 0)
        {
            norm_attribute = &primitive->attributes[i];
        }
    }

    if (!pos_attribute || !tex_attribute || !norm_attribute)
    {
        printl(Log::LogLevel::Warn, "[CGLTF] Missing attributes in primitive");
        return;
    }

    size_t vertexCount = pos_attribute->data->count;
    size_t indexCount = primitive->indices->count;

    for (size_t i = 0; i < vertexCount; i++)
    {
        Vertex vertex = {};

        // Read original vertex data
        if (cgltf_accessor_read_float(pos_attribute->data, i, &vertex._position.x, 3) == 0)
        {
            printl(Log::LogLevel::Warn, "[CGLTF] Unable to read Position attributes!");
        }
        if (cgltf_accessor_read_float(tex_attribute->data, i, &vertex._texCoord.x, 2) == 0)
        {
            printl(Log::LogLevel::Warn, "[CGLTF] Unable to read Texture attributes!");
        }
        if (cgltf_accessor_read_float(norm_attribute->data, i, &vertex._normal.x, 3) == 0)
        {
            printl(Log::LogLevel::Warn, "[CGLTF] Unable to read Normal attributes!");
        }

        tempVertices.push_back(vertex);
    }

    for (int i = 0; i < indexCount; i++)
    {
        tempIndices.push_back(cgltf_accessor_read_index(primitive->indices, i));
    }

    // material

    cgltf_material* material = primitive->material;
    if (!model._materialLookup.contains(material)) // if the hash table doesn't have the material hash
    {
        Material mat = {};

        HRESULT hr = E_FAIL;

        // map texture types to their respective textures
        std::unordered_map<TextureType, cgltf_texture_view*> textureMap;

        // the following code for materials is very much unoptimised.
        // It should only look for materials once, make a texture, sampler and save it in a map, not per primitive
        // TODO -- RESOLVED

        // it still is prolly unoptimised due to too much use of hashmaps and string ops everywhere
        // need to find a better solution (nvtt3?)

        if (material->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
            // Map base color texture (albedo)
            if (pbr->base_color_texture.texture)
            {
                textureMap[TextureType::ALBEDO] = &pbr->base_color_texture;
            }
            if (pbr->metallic_roughness_texture.texture)
            {
                // Map metallic-roughness texture
                textureMap[TextureType::METALLIC_ROUGHNESS] = &pbr->metallic_roughness_texture;
                //mat.MaterialName = 
            }
        }

        if (material->normal_texture.texture)
        {
            // Map normal texture
            textureMap[TextureType::NORMAL] = &material->normal_texture;
        }
        if (material->emissive_texture.texture)
        {
            // Map emissive texture
            textureMap[TextureType::EMISSIVE] = &material->emissive_texture;
        }
        if (material->has_pbr_specular_glossiness)
        {
            cgltf_pbr_specular_glossiness* pbr = &material->pbr_specular_glossiness;
            if (pbr->specular_glossiness_texture.texture)
            {
                //textureMap[TextureType::SPECULAR_GLOSSINESS] = &material->tex
            }
        }

        // Load all textures from the map if they haven't been loaded before
        for (const auto& [type, view] : textureMap)
        {
            std::string imageName = view->texture->image->uri;
            u32 textureIndex = -1;

            if (!model._loadedTextures.contains(imageName)) // If texture file is new
            {
                textureIndex = LoadMaterialTexture(gfxDevice, frameSync, model, mat, view, type);

                // Cache the index for this image file
                model._loadedTextures.insert(imageName);
                model._textureIndexLookup[imageName] = textureIndex;
                printl(Log::LogLevel::Info, "[Texture] Loaded texture {} with index {}", imageName, textureIndex);
            }
            else
            {
                textureIndex = model._textureIndexLookup[imageName];
                printl(Log::LogLevel::Warn, "[Texture] Reusing texture {} with index {}", imageName, textureIndex);

                // there is no image view at model side, SRV is created with heap in d3d12, so leave it alone here

                //Texture& existingTex = model._modelTextures[textureIndex];
                //if (type == TextureType::ALBEDO) mat._AlbedoView = existingTex._imageView;
                //if (type == TextureType::NORMAL) mat.NormalView = existingTex._imageView;
                //if (type == TextureType::METALLIC_ROUGHNESS) mat.MetallicRoughnessView = existingTex._imageView;
                //if (type == TextureType::EMISSIVE) mat.EmissiveView = existingTex._imageView;
                // ... rest types TODO
            }

            if (type == TextureType::ALBEDO) mat._albedoIndex = textureIndex;
            if (type == TextureType::NORMAL) mat._normalIndex = textureIndex;
            if (type == TextureType::METALLIC_ROUGHNESS) mat._metallicIndex = textureIndex;
            if (type == TextureType::EMISSIVE) mat._emmisiveIndex = textureIndex;
            // ... rest types TODO
        }

        model._materials.push_back(mat);
        model._materialLookup[material] = model._materials.size() - 1;
    }
    meshInfo._materialIndex = static_cast<u32>(model._materialLookup[material]);
    meshInfo._transform = parentTransform;
    meshInfo._startIndex = indexOffset;
    meshInfo._startVertex = vertexOffset;
    meshInfo._vertexCount = vertexCount;
    meshInfo._indexCount = indexCount;

    Mesh mesh;
    mesh._vertices = tempVertices;
    mesh._indices = tempIndices;
    mesh._vertexCount = static_cast<u32>(vertexCount);
    mesh._indexCount = static_cast<u32>(indexCount);

    for (int i = 0; i < tempVertices.size(); i++) model._vertices.push_back(tempVertices[i]);
    for (int i = 0; i < tempIndices.size(); i++) model._indices.push_back(tempIndices[i]);

    //OptimiseMesh(meshInfo, mesh);
#if MESH_SHADING
    ProcessMeshlets(mesh);
#endif
    model._meshes.push_back(meshInfo);
}

static void ProcessNode(GfxDevice& gfxDevice, FrameSync& frameSync, cgltf_node* node,
    Model& model, Transformation& parentTransform)
{
    Transformation localTransform{};
    localTransform._matrix = parentTransform._matrix;

    if (node->has_matrix) 
    {
        //localTransform._matrix *= //make matrix from node->matrix;
    }
    else 
    {
        if (node->has_translation)
        {
            localTransform._matrix *= DirectX::XMMatrixTranslation(node->translation[0], node->translation[1], node->translation[2]);
        }
        if (node->has_rotation)
        {
            DirectX::XMVECTOR quat = DirectX::XMVectorSet(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
            localTransform._matrix *= DirectX::XMMatrixRotationQuaternion(quat);
        }
        if (node->has_scale)
        {
            localTransform._matrix *= DirectX::XMMatrixScaling(node->scale[0], node->scale[1], node->scale[2]);
        }
    }

    if (node->mesh)
    {
        for (size_t i = 0; i < (node->mesh->primitives_count); i++)
        {
            ProcessPrimitive(gfxDevice, frameSync, &node->mesh->primitives[i], model, localTransform);
        }
    }

    // Recursively process child nodes
    for (size_t i = 0; i < node->children_count; i++)
    {
        ProcessNode(gfxDevice, frameSync, node->children[i], model, localTransform);
    }
}

static void SetResources(GfxDevice& gfxDevice, Model& model)
{
    model._vertexBuffer = CreateBuffer(gfxDevice,
        {
        ._bufferSize = u32(sizeof(Vertex) * model._vertices.size()),
        ._bufferType = BufferType::VERTEX,
        ._pContents = model._vertices.data() });

    model._indexBuffer = CreateBuffer(gfxDevice,
        {
        ._bufferSize = u32(sizeof(model._indices[0]) * model._indices.size()),
		._bufferType = BufferType::INDEX,
        ._pContents = model._indices.data() });

    const u32 descriptorSize = gfxDevice._device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(model._modelHeap->GetCPUDescriptorHandleForHeapStart());

    for (auto& texture : model._modelTextures)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = texture._resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = texture._resource->GetDesc().MipLevels;

        gfxDevice._device->CreateShaderResourceView(texture._resource.Get(), &srvDesc, srvHandle);
        srvHandle.Offset(descriptorSize);
    }
}

static void ValidateResources()
{
	
}

static void SetTextureHeap(const GfxDevice& gfxDevice, Model& model)
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.NumDescriptors = MAX_TEXTURES;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    DX_ASSERT(gfxDevice._device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&model._modelHeap)));
}

Model LoadModel(GfxDevice& gfxDevice, FrameSync& frameSync, ModelDesc desc)
{
	Model model{};

    // allocate srv heap for textures
    SetTextureHeap(gfxDevice, model);

	cgltf_options options{};
	cgltf_data* data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, desc._path.c_str(), &data);

    if (result != cgltf_result_success)
        printl(Log::LogLevel::Error, "[CGLTF] Failed to parse gltf file");
    else
        printl(Log::LogLevel::Info, "[CGLTF] Successfully parsed gltf file");

    result = cgltf_load_buffers(&options, data, desc._path.c_str());

    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        printl(Log::LogLevel::Error, "[CGLTF] Failed to load buffers");
    }
    else
    {
        printl(Log::LogLevel::Info, "[CGLTF] Successfully loaded buffers");
    }

    cgltf_scene* scene = data->scene;

    if (!scene)
    {
        printl(Log::LogLevel::Error, "[CGLTF] No scene found in gltf file");
    }
    else
    {
        printl(Log::LogLevel::Info, "[CGLTF] Scene found in gltf file");
        model._dirPath = desc._path.substr(0, desc._path.find_last_of("/"));

        for (size_t i = 0; i < (scene->nodes_count); i++)
        {
            Transformation transform;
            ProcessNode(gfxDevice, frameSync, scene->nodes[i], model, transform);
        }
        // no of nodes
        printl(Log::LogLevel::InfoDebug, "[CGLTF] No of nodes in the scene: {} ", scene->nodes_count);

        SetResources(gfxDevice, model);

        printl(Log::LogLevel::Info, "[CGLTF] Successfully loaded gltf file");
    }
    //ValidateResources();
    cgltf_free(data);

	return model;
}
