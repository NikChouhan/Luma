#include "Texture.h"

#include <D3D12MemAlloc.h>

#include "FrameSync.h"
#include "GfxDevice.h"

Texture CreateTexture(GfxDevice& gfxDevice, FrameSync& frameSync, TextureDesc desc)
{
	Texture texture;

	{
		/*
		 * 1. Create the texture resource
		 * 2. Take in the texture buffer data from desc
		 * (some textures may not have a data, so the pointer must be nullptr else)
		 * 3. if data present, map the data to the texture resource
		 * (set the upload type as HEAP_TYPE_GPU_UPLOAD for the resizable bar)
		 * 4. Profit...?
		*/

		// step 1
		CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			desc._texWidth,
			desc._texHeight,
			1,
			0,
			1,
			0,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE);


		D3D12MA::CALLOCATION_DESC allocDesc = D3D12MA::CALLOCATION_DESC
		{
			D3D12_HEAP_TYPE_GPU_UPLOAD,
			D3D12MA::ALLOCATION_FLAG_STRATEGY_BEST_FIT
		};

 		D3D12MA::Allocation* textureAllocation{};
		DX_ASSERT(gfxDevice._allocator->CreateResource(&allocDesc, &textureDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr, &textureAllocation, IID_NULL, nullptr));
		texture._resource = textureAllocation->GetResource();
		//textureAllocation->Release();

		//// manual allocation path
		//D3D12_RESOURCE_DESC textureDesc{};
		//textureDesc.MipLevels = 0;
		//textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//textureDesc.Width = desc._texWidth;
		//textureDesc.Height = desc._texHeight;
		//textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		//textureDesc.DepthOrArraySize = 1;
		//textureDesc.SampleDesc.Count = 1;
		//textureDesc.SampleDesc.Quality = 0;
		//textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		//textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		//auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_GPU_UPLOAD);

		//DX_ASSERT(gfxDevice._device->CreateCommittedResource(&heapProps,
		//	D3D12_HEAP_FLAG_NONE, &textureDesc,
		//	D3D12_RESOURCE_STATE_COMMON, 
		//	nullptr,
		//	IID_PPV_ARGS(&texture._resource)));


		// step 2 & step 3

		u32 bufferSize = desc._texHeight * desc._texWidth * desc._texPixelSize;
		CD3DX12_RANGE readRange(0, 0);
		
		DX_ASSERT(texture._resource->Map(0, &readRange, nullptr));
		DX_ASSERT(texture._resource->WriteToSubresource(0, nullptr, desc._pContents,
			desc._texWidth * desc._texPixelSize,
			desc._texWidth * desc._texPixelSize * desc._texHeight));
		//memcpy(*pDataBegin, desc._pContents, bufferSize);

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		DX_ASSERT(gfxDevice._device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&texture._srvHeap)));

		/* do the srv tasks, then close the command list and then execute it.
		 Use the fence objects to ensure the upload tasks are done b4 it
		 proceeds with the later commands in the current queue (direct queue here)
		 i.e just use the WaitforGPU function
		*/

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		gfxDevice._device->CreateShaderResourceView(texture._resource.Get(), &srvDesc, texture._srvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	ImmediateSubmit(gfxDevice, frameSync, [&](ComPtr<ID3D12GraphicsCommandList1> commandList)
		{
			CD3DX12_RESOURCE_BARRIER pRBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture._resource.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &pRBarrier);
		});


	return texture;
}


