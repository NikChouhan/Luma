#pragma once
#include <variant>

#include "Graphics/GfxDevice.h"
#include "Graphics/D3D12/Shader.h"

enum class BlendMode : u8 { NON_TRANSPARENT, ALPHA_BLEND, ADDITIVE, PREMULTIPLIED };
enum class DepthMode : u8 { READ_WRITE, READ_ONLY, NONE };
enum class RasterMode : u8 { SOLID_BACK_CULL, SOLID_NONE_CULL, WIREFRAME };
enum class Topology : u8 { TRIANGLES, LINES, POINTS };

enum class PipelineType : u8 { GRAPHICS, COMPUTE };

struct PipelineHandle
{
	u32 index : 24;
	u32 generation : 8;

	bool operator==(const PipelineHandle& other) const
	{
		return index == other.index && generation == other.generation;
	}
};

static constexpr PipelineHandle g_invalidPipelineHandle = { .index = 0xFFFFFF, .generation = 0xFF };


struct GraphicsPipelineDesc
{
	ShaderHandle vertexShader = g_invalidShaderHandle;
	ShaderHandle pixelShader = g_invalidShaderHandle;

	BlendMode blendMode = BlendMode::NON_TRANSPARENT;
	DepthMode depthMode = DepthMode::READ_WRITE;
	RasterMode rasterMode = RasterMode::SOLID_BACK_CULL;
	Topology topology = Topology::TRIANGLES;

	// render targets
	
	DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

	// optional (I will keep an input layout set always, if none provided)
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
};

struct ComputePipelineDesc
{
	ShaderHandle computeShader = g_invalidShaderHandle;
};

struct Pipeline
{
	Pipeline() = default;
	~Pipeline() = default;

	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12RootSignature> rootSign;
	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	void Bind(ID3D12GraphicsCommandList* cmdList) const
	{
		if (!pso) return;
		cmdList->SetPipelineState(pso.Get());
		//cmdList->SetGraphicsRootSignature(rootSign.Get());
		cmdList->IASetPrimitiveTopology(primitiveTopology);
	}

	void Release()
	{
		if (pso) {
			pso.Reset();
		}
		if (pso) {
			pso.Reset();
		}
	}
};