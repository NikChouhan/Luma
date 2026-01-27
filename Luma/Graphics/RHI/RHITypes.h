#pragma once

#include "Core/Common.h"

#include <variant>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;
	//DirectX::XMFLOAT4 tangent;
};

template<typename T>
struct RHIHandle
{
	u32 index = 0xFFFFFFFF;
	u32 generation = 0;
	bool IsValid() const { return index != 0xFFFFFFFF; }
	bool operator==(const RHIHandle& other) const
	{
		return index == other.index && generation == other.generation;
	};
};

using BufferHandle = RHIHandle<struct BufferTag>;
using TextureHandle = RHIHandle<struct TextureTag>;
using ShaderHandle = RHIHandle<struct ShaderTag>;
using PipelineHandle = RHIHandle<struct PipelineTag>;

static constexpr BufferHandle g_invalidBufferHandle = { .index = 0xFFFFFF, .generation = 0xFF };
static constexpr TextureHandle g_invalidTextureHandle = { .index = 0xFFFFFF, .generation = 0xFF };
static constexpr ShaderHandle g_invalidShaderHandle = { .index = 0xFFFFFF, .generation = 0xFF };
static constexpr PipelineHandle g_invalidPipelineHandle = { .index = 0xFFFFFF, .generation = 0xFF };

enum class RHIFormat : u8
{
	UNKNOWN,
	R32G32B32_FLOAT,
	R32G32_FLOAT,
	R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB,
	R16G16B16A16_FLOAT,
	R32G32B32A32_FLOAT,
	D32_FLOAT,
	R16_UINT,
	R32_UINT,
	R32_TYPELESS
	// add others depending on need
};

enum class RHIBlendMode : u8 { NON_TRANSPARENT, ALPHA_BLEND, ADDITIVE, PREMULTIPLIED };
enum class RHIDepthFunc : u8 { NONE, NEVER, LESS, EQUAL, LEQUAL, GREATER, NEQUAL, GEQUAL, ALWAYS };
enum class RHIDepthMode : u8 { NONE, READ, WRITE };
enum class RHIRasterMode : u8 { NONE, FRONT, BACK, WIREFRAME };
enum class RHITopology : u8 { TRIANGLE_LIST, LINE_LIST, POINT_LIST };
enum class RHIShaderStage : u8 { VERTEX, PIXEL, MESH, COMPUTE };

enum class RHIMemoryeUsage
{
	UPLOAD,      // CPU write, GPU read (default for VB/IB/CB)
	DEFAULT,     // GPU only
	READBACK,    // GPU write, CPU read
	GPU_UPLOAD   // CPU available (and writeable, don't read ever, its slow af), GPU located memory
};

// buffers
enum class RHIResourceView : u8
{
	LOAD,
	STORE,
	RENDER_TARGET,
	DEPTH_STENCIL
};
inline RHIResourceView operator|(RHIResourceView a, RHIResourceView b)
{
	return static_cast<RHIResourceView>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline bool HasFlag(RHIResourceView flags, RHIResourceView check)
{
	return (static_cast<u32>(flags) & static_cast<u32>(check)) != 0;
}

struct RHIVertexBufferCreateInfo
{
	const void* vertices;
	u32 vertexCount;
	u32 vertexStride;
};

struct RHIIndexBufferCreateInfo
{
	const void* indices;
	u32 indexCount;
	RHIFormat format = RHIFormat::R32_UINT;
};

struct RHIConstantBufferCreateInfo
{
	const void* data;
	u32 sizeInBytes;
	bool createView = true; // view created and set in descriptor heap
};

struct RHIStructuredBufferCreateInfo
{
	const void* data;
	u32 elementCount;
	u32 elementStride;
};

struct RHIRawBufferCreateInfo
{
	const void* data;
	u32 sizeInBytes;
	RHIFormat format = RHIFormat::R32_TYPELESS;
};

using RHIBufferCreateInfo = std::variant<
	RHIVertexBufferCreateInfo,
	RHIIndexBufferCreateInfo,
	RHIConstantBufferCreateInfo,
	RHIStructuredBufferCreateInfo,
	RHIRawBufferCreateInfo
>;

struct RHIBufferDesc
{
	RHIBufferCreateInfo createInfo;
	RHIMemoryeUsage usage = RHIMemoryeUsage::DEFAULT;
	RHIResourceView view = RHIResourceView::LOAD | RHIResourceView::STORE;
	const wchar_t* debugName = nullptr;
};

// textures
struct RHITextureDesc
{
	u32 width = 1920;
	u32 height = 1080;
	u32 depth = 0;
	u32 mips = 1;
	u32 arraySize = 1;
	RHIFormat format = RHIFormat::R8G8B8A8_UNORM;
	RHIMemoryeUsage usage = RHIMemoryeUsage::DEFAULT;
	RHIResourceView view = RHIResourceView::LOAD;

	// create UAV per mip level (for mip generation)
	// TODO: Do 
	bool createPerMipViews;
	const wchar_t* debugName = nullptr;
};

// shaders
struct RHIShaderDesc 
{
	const wchar_t* path;
	const wchar_t* entryPoint;
	const wchar_t* target;
	RHIShaderStage stage;
};

// pipelines
struct RHIComputePipelineDesc
{
	ShaderHandle cs;
};

enum class RHIInputRate : u8 
{
	PerVertex,
	PerInstance
};

struct RHIInputBindingDesc
{
	u32 binding = 0;
	u32 stride = 0;
	RHIInputRate inputRate = RHIInputRate::PerVertex;
	u32 instanceStepRate = 0;
};

struct RHIInputAttributeDesc
{
	u32 location = 0;
	u32 binding = 0;
	RHIFormat format = RHIFormat::UNKNOWN;
	u32 offset = 0;
	std::string semanticName = ""; // for DX12 only
	u32 semanticIndex = 0;	// Dx12 only
};

inline std::vector<RHIInputBindingDesc> kBindingdescs =
{ {.binding = 0, .stride = sizeof(Vertex), .inputRate = RHIInputRate::PerVertex } };

inline std::vector<RHIInputAttributeDesc> kInputAttributes = {
	{.location = 0, .binding = 0, .format = RHIFormat::R32G32B32_FLOAT, .offset = offsetof(Vertex, position), .semanticName = "POSITION" },
	{.location = 1, .binding = 0, .format = RHIFormat::R32G32_FLOAT, .offset = offsetof(Vertex, texCoord), .semanticName = "TEXCOORD" },
	{.location = 2, .binding = 0, .format = RHIFormat::R32G32B32_FLOAT, .offset = offsetof(Vertex, normal), .semanticName = "NORMAL" },
	//{.location = 3, .binding = 0, .format = RHIFormat::R32G32B32A32_FLOAT, .offset = offsetof(Vertex, tangent), .semanticName = "TANGENT" }
};
struct RHIGraphicsPipelineDesc 
{
	ShaderHandle vs;
	ShaderHandle ps;
	RHIBlendMode blend = RHIBlendMode::NON_TRANSPARENT;
	RHIDepthFunc depthFunc = RHIDepthFunc::LESS;
	RHIDepthMode depthMode = RHIDepthMode::READ;
	RHIRasterMode rasterMode = RHIRasterMode::BACK;
	RHITopology topology = RHITopology::TRIANGLE_LIST;

	std::vector<RHIFormat> colorFormats;
	RHIFormat depthFormat = RHIFormat::UNKNOWN;

	std::vector<RHIInputBindingDesc> inputBindings;
	std::vector<RHIInputAttributeDesc> inputAttributes;
};

// for barriers/resource transition
enum class RHIResourceState : u8 
{
	COMMON, VERTEX_BUFFER, INDEX_BUFFER,
	RENDER_TARGET, DEPTH_WRITE, DEPTH_READ,
	UNORDERED_ACCESS, SHADER_RESOURCE, COPY_DEST, COPY_SOURCE, PRESENT
};

struct RHIViewPort
{
	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;

	// Optional: convenience constructor
	RHIViewPort() : x(0), y(0), width(0), height(0), minDepth(0.0f), maxDepth(1.0f) {}
	RHIViewPort(float x, float y, float w, float h, float minD = 0.0f, float maxD = 1.0f)
		: x(x), y(y), width(w), height(h), minDepth(minD), maxDepth(maxD) {
	}
};

struct RHIScissor
{
	int x;
	int y;
	int width;
	int height;

	RHIScissor() : x(0), y(0), width(0), height(0) {}
	RHIScissor(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
};