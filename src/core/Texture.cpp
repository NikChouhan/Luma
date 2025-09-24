#include "Texture.h"
#include "FrameSync.h"

#include <D3D12MemAlloc.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

Texture CreateTexture(GfxDevice& gfxDevice, FrameSync& frameSync, TextureDesc desc)
{
	Texture texture;

	// Method 1: with REBAR
	//{
		/*
		 * 1. Create the texture resource
		 * 2. Take in the texture buffer data from desc
		 * (some textures may not have a data, so the pointer must be nullptr else)
		 * 3. if data present, map the data to the texture resource
		 * (set the upload type as HEAP_TYPE_GPU_UPLOAD for the resizable bar)
		 * 4. Profit...?
		*/

		// step 1
	//	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
	//		DXGI_FORMAT_R8G8B8A8_UNORM,
	//		desc._texWidth,
	//		desc._texHeight,
	//		1,
	//		0,
	//		1,
	//		0,
	//		D3D12_RESOURCE_FLAG_NONE,
	//		D3D12_TEXTURE_LAYOUT_UNKNOWN);


	//	D3D12MA::CALLOCATION_DESC allocDesc = D3D12MA::CALLOCATION_DESC
	//	{
	//		D3D12_HEAP_TYPE_GPU_UPLOAD,
	//		D3D12MA::ALLOCATION_FLAG_STRATEGY_BEST_FIT
	//	};

 //		D3D12MA::Allocation* textureAllocation{};
	//	DX_ASSERT(gfxDevice._allocator->CreateResource(&allocDesc, &textureDesc,
	//		D3D12_RESOURCE_STATE_COMMON,
	//		nullptr, &textureAllocation, IID_NULL, nullptr));
	//	texture._resource = textureAllocation->GetResource();
	//	//textureAllocation->Release();

	//	//// manual allocation path
	//	//D3D12_RESOURCE_DESC textureDesc{};
	//	//textureDesc.MipLevels = 0;
	//	//textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//	//textureDesc.Width = desc._texWidth;
	//	//textureDesc.Height = desc._texHeight;
	//	//textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	//	//textureDesc.DepthOrArraySize = 1;
	//	//textureDesc.SampleDesc.Count = 1;
	//	//textureDesc.SampleDesc.Quality = 0;
	//	//textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	//	//textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//	//auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_GPU_UPLOAD);

	//	//DX_ASSERT(gfxDevice._device->CreateCommittedResource(&heapProps,
	//	//	D3D12_HEAP_FLAG_NONE, &textureDesc,
	//	//	D3D12_RESOURCE_STATE_COMMON, 
	//	//	nullptr,
	//	//	IID_PPV_ARGS(&texture._resource)));


	//	// step 2 & step 3

	//	u32 bufferSize = desc._texHeight * desc._texWidth * desc._texPixelSize;
	//	CD3DX12_RANGE readRange(0, 0);
	//	
	//	DX_ASSERT(texture._resource->Map(0, &readRange, nullptr));
	//	DX_ASSERT(texture._resource->WriteToSubresource(0, nullptr, desc._pContents,
	//		desc._texWidth * desc._texPixelSize,
	//		desc._texWidth * desc._texPixelSize * desc._texHeight));
	//	//memcpy(*pDataBegin, desc._pContents, bufferSize);


	//	/*
	//	 Use the fence objects to ensure the upload tasks are done b4 it
	//	 proceeds with the later commands in the current queue (direct queue here)
	//	 i.e just use the WaitforGPU function
	//	*/
	//}
	//ImmediateSubmit(gfxDevice, frameSync, [&](ComPtr<ID3D12GraphicsCommandList1> commandList)
	//	{
	//		CD3DX12_RESOURCE_BARRIER pRBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture._resource.Get(),
	//			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//		commandList->ResourceBarrier(1, &pRBarrier);
	//	});

	// Method 2: With the Upload/Default heap combo

	// Create the texture.
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = desc._texWidth;
	textureDesc.Height = desc._texHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	DX_ASSERT(gfxDevice._device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture._resource)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture._resource.Get(),
		0, 1);
	ComPtr<ID3D12Resource> textureUploadHeap;
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// Create the GPU upload buffer.
	DX_ASSERT(gfxDevice._device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = desc._pContents;
	textureData.RowPitch = desc._texWidth * desc._texPixelSize;
	textureData.SlicePitch = textureData.RowPitch * desc._texHeight;

	ImmediateSubmit(gfxDevice, frameSync, [&](ComPtr<ID3D12GraphicsCommandList1> commandList)
		{
			UpdateSubresources(commandList.Get(), texture._resource.Get(),
				textureUploadHeap.Get(), 0, 0, 1, &textureData);
			auto pBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture._resource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &pBarrier);
		});
	return texture;
}


