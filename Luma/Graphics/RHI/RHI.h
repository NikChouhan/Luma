#pragma once
#include <type_traits>

#include "RHITypes.h"
#include "Graphics/Globals.h"


#if defined(RHI_BACKEND_D3D12)
#include "Graphics/RHI/D3D12/D3D12Backend.h"
using RHIBackend = D3D12Backend;
using BackendCommandList = D3D12Backend::D3D12BackendCommandList;
#elif RHI_BACKEND_VULKAN
#include "Graphics/RHI/Vulkan/VulkanBackend.h"
using RHIBackend = VulkanBackend;
using BackendCommandList = VulkanBackend::VulkanBackendCommandList;
#endif

struct RHI
{
    static void Init(SDL_Window* window, u32 width, u32 height)
    {
        RHIBackend::Init(window, width, height);
    }
    static void Shutdown()
    {
        RHIBackend::Shutdown();
    }
    static void BeginFrame()
    {
        RHIBackend::BeginFrame();
    }
    static void WaitIdle()
    {
        RHIBackend::WaitIdle();
    }
    // resource
    static BufferHandle CreateBuffer(const RHIBufferDesc& desc, const void* initialData = nullptr)
    {
        return RHIBackend::CreateBuffer(desc, initialData);
    }
    static TextureHandle CreateTexture(const RHITextureDesc& desc, const void* initialData = nullptr)
    {
        return RHIBackend::CreateTexture(desc, initialData);
    }
    static ShaderHandle CreateShader(const RHIShaderDesc& desc)
    {
        return RHIBackend::CreateShader(desc);
    }
    static PipelineHandle CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc)
    {
        return RHIBackend::CreateGraphicsPipeline(desc);
    }
    static PipelineHandle CreateComputePipeline(const RHIComputePipelineDesc& desc)
    {
        return RHIBackend::CreateComputePipeline(desc);
    }
    static void DestroyBuffer(BufferHandle handle)
    {
        RHIBackend::DestroyBuffer(handle);
    }
    static void DestroyTexture(TextureHandle handle)
    {
        RHIBackend::DestroyTexture(handle);
    }
    static void DestroyPipeline(PipelineHandle handle)
    {
        RHIBackend::DestroyPipeline(handle);
    }
    // bindless stuff
    static i32 GetBindlessReadIndex(TextureHandle handle) // SRV/sampled index (-1 if invalid)
    {
	    return RHIBackend::GetBindlessReadIndex(handle);
    }
    static i32 GetBindlessWriteIndex(TextureHandle handle) // UAV/storage index
    {
        return RHIBackend::GetBindlessWriteIndex(handle);
    }

    static i32 GetBindlessReadIndex(BufferHandle handle)   // SRV/CBV/SSBO read (uniform texel/buffer for Vulkan)
    {
        return RHIBackend::GetBindlessReadIndex(handle);
    }
    static i32 GetBindlessWriteIndex(BufferHandle handle)  // UAV/SSBO write (storage texel/buffer for Vulkan)
    {
        return RHIBackend::GetBindlessWriteIndex(handle);
    }
    // commands
    struct CommandList
    {
    public:
        CommandList(bool isImmediate) : backendData(RHIBackend::CreateCommandList(isImmediate)) {}
        ~CommandList() { RHIBackend::DestroyCommandList(static_cast<BackendCommandList*>(backendData)); }

        void Begin()
        {
            return static_cast<BackendCommandList*>(backendData)->Begin();
        }
        void End()
        {
            return static_cast<BackendCommandList*>(backendData)->End();
        }
        void Submit()
        {
            return static_cast<BackendCommandList*>(backendData)->Submit();
        }
        void TextureBarrier(TextureHandle handle, RHIResourceState before, RHIResourceState after)
        {
            return static_cast<BackendCommandList*>(backendData)->TextureBarrier(handle, before, after);
        }
        void BufferBarrier(BufferHandle handle, RHIResourceState before, RHIResourceState after)
        {
            return static_cast<BackendCommandList*>(backendData)->BufferBarrier(handle, before, after);
        }
        void SetPipeline(PipelineHandle handle)
        {
            return static_cast<BackendCommandList*>(backendData)->SetPipeline(handle);
        }
        void SetGraphicsPushConstants(const void* data, u32 size, u32 offset)
        {
            return static_cast<BackendCommandList*>(backendData)->SetGraphicsPushConstants(data, size, offset);
        }
        void BeginRendering(const std::vector<TextureHandle> colorTargets, TextureHandle depthTarget)
        {
            return static_cast<BackendCommandList*>(backendData)->BeginRendering(colorTargets, depthTarget);
        }
        void EndRendering()
        {
            return static_cast<BackendCommandList*>(backendData)->EndRendering();
        }
        void SetViewport(const RHIViewPort& viewPort)
        {
            return static_cast<BackendCommandList*>(backendData)->SetViewport(viewPort);
        }
        void SetScissor(const RHIScissor& scissor)
        {
            return static_cast<BackendCommandList*>(backendData)->SetScissor(scissor);
        }
        void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
        {
            return static_cast<BackendCommandList*>(backendData)->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
        }
        void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
        {
            return static_cast<BackendCommandList*>(backendData)->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }
        void Dispatch(u32 x, u32 y, u32 z)
        {
            return static_cast<BackendCommandList*>(backendData)->Dispatch(x, y, z);
        }
        void CopyBufferToTexture(BufferHandle src, TextureHandle dst)
        {
            return static_cast<BackendCommandList*>(backendData)->CopyBufferToTexture(src, dst);
        }
        void CopyBuffer(BufferHandle src, BufferHandle dst, u64 size, u64 srcOffset, u64 dstOffset)
        {
            return static_cast<BackendCommandList*>(backendData)->CopyBuffer(src, dst, size, srcOffset, dstOffset);
        }
        void BindVertexBuffer(BufferHandle handle)
        {
            return static_cast<BackendCommandList*>(backendData)->BindVertexBuffer(handle);
        }
        void BindIndexBuffer(BufferHandle handle)
        {
            return static_cast<BackendCommandList*>(backendData)->BindIndexBuffer(handle);
        }
    private:
        void* backendData = nullptr;
    };
    static void EndFrame()
    {
        RHIBackend::EndFrame();
    }

    static CommandList* CreateCommandList(bool isImmediate = false) { return new CommandList(isImmediate); }
    static void DestroyCommandList(CommandList* cl) { delete cl; }

    template<typename Func>
    static void ImmediateSubmit(Func&& callback) {
        auto cl = CreateCommandList(true);
        cl->Begin();
        callback();
        cl->End();
        cl->Submit();
        //DestroyCommandList(cl);
    }
    

    //static void ImmediateSubmit(ImmediateContext* immediateCtx, LAMBDA() callback);
    // unused patth, might need in the future
    // Command List Access 
    //template<typename Func>
    //static void ExecuteCommands(Func&& func)
    //{
    //    RHIBackend::ExecuteCommands(std::forward<Func>(func));
    //}

};