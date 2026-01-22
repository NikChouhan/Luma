#pragma once
#include <type_traits>
#include "RHICommandList.h"

#include "Graphics/Globals.h"

#ifdef _WIN32
#define RHI_BACKEND_D3D12
#elif LINUX
#define RHI_BACKEND_VULKAN
#endif

#if defined(RHI_BACKEND_D3D12)
#include "Graphics/RHI/D3D12/D3D12Backend.h"
using RHIBackend = D3D12Backend;
#elif RHI_BACKEND_VULKAN
#include "Vulkan/VulkanBackend.h"
using RHIBackend = VulkanBackend;
#else
#error "No RHI backend defined"
#endif

struct CommandList 
{
    void* backendData = nullptr;
};

struct RHI
{
    static void Init(void* windowHandle, u32 width, u32 height)
    {
        RHIBackend::Init(windowHandle, width, height);
    }
    static void Shutdown()
    {
        RHIBackend::Shutdown();
    }
    static void BeginFrame()
    {
        RHIBackend::BeginFrame();
    }
    static void EndFrame()
    {
        RHIBackend::EndFrame();
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
        CommandList() : backendData(RHIBackend::CreateCommandList()) {}
        ~CommandList() { RHIBackend::DestroyCommandList(static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)); }

        void Begin()
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->Begin();
        }
        void End()
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->End();
        }
        void Submit()
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->Submit();
        }
        void TextureBarrier(TextureHandle handle, ResourceState before, ResourceState after)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->TextureBarrier(handle, before, after);
        }
        void BufferBarrier(BufferHandle handle, ResourceState before, ResourceState after)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->BufferBarrier(handle, before, after);
        }
        void SetPipeline(PipelineHandle handle)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->SetPipeline(handle);
        }
        void SetPushConstants(const void* data, u32 size, u32 offset)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->SetPushConstants(data, size, offset);
        }
        void BeginRendering(const std::span<TextureHandle> colorTargets, TextureHandle depthTarget)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->BeginRendering(colorTargets, depthTarget);
        }
        void EndRendering()
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->EndRendering();
        }
        void SetViewport(float x, float y, float w, float h, float minD, float maxD)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->SetViewport(x, y, w, h, minD, maxD);
        }
        void SetScissor(int x, int y, int w, int h)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->SetScissor(x, y, w, h);
        }
        void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
        }
        void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }
        void Dispatch(u32 x, u32 y, u32 z)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->Dispatch(x, y, z);
        }
        void CopyBufferToTexture(BufferHandle src, TextureHandle dst)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->CopyBufferToTexture(src, dst);
        }
        void CopyBuffer(BufferHandle src, BufferHandle dst, u64 size, u64 srcOffset, u64 dstOffset)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->CopyBuffer(src, dst, size, srcOffset, dstOffset);
        }
        void BindVertexBuffer(BufferHandle handle)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->BindVertexBuffer(handle);
        }
        void BindIndexBuffer(BufferHandle handle)
        {
            return static_cast<RHIBackend::D3D12BackendCommandList*>(backendData)->BindIndexBuffer(handle);
        }
    private:
        void* backendData = nullptr;
    };

    static CommandList* CreateCommandList() { return new CommandList(); }
    static void DestroyCommandList(CommandList* cl) { delete cl; }

    template<typename Func>
    static void ImmediateSubmit(Func&& callback) {
        auto cl = CreateCommandList();
        cl->Begin();
        callback(*cl);
        cl->End();
        cl->Submit();
        DestroyCommandList(cl);
    }

    struct ImmediateContext;

    static void ImmediateSubmit(ImmediateContext* immediateCtx, LAMBDA() callback);
    // unused patth, might need in the future
    // Command List Access 
    //template<typename Func>
    //static void ExecuteCommands(Func&& func)
    //{
    //    RHIBackend::ExecuteCommands(std::forward<Func>(func));
    //}

};