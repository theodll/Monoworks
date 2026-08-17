#pragma once
#include <common/Base.hh>

#include <rhi/Utils.hh>

#include <rhi/agnostic/VertexBuffer.hh>

namespace Monoworks::RHI 
{
	enum EPrimitiveTopology : u8
	{
		MW_PRIMITIVE_TOPOLOGY_UNKOWN, 
		
		MW_PRIMITIVE_TOPOLOGY_POINT_LIST,
		MW_PRIMITIVE_TOPOLOGY_LINE_LIST,
		MW_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		
		MW_PRIMITIVE_TOPOLOGY_COUNT
	};

	enum ECompareOp 
	{
		MW_COMPARE_OP_NEVER,

		MW_COMPARE_OP_LESS,
		MW_COMPARE_OP_EQUAL,
		MW_COMPARE_OP_LESS_OR_EQUAL,
		MW_COMPARE_OP_GREATER,
		MW_COMPARE_OP_NOT_EQUAL,
		MW_COMPARE_OP_GREATER_OR_EQUAL,
		MW_COMPARE_OP_ALWAYS,

		MW_COMPARE_OP_MAX_ENUM = 0x7FFFFFFF
	};

	enum EDynamicState : u8 
	{
		MW_DYNAMIC_STATE_VIEWPORT,
		MW_DYNAMIC_STATE_SCISSOR,
		MW_DYNAMIC_STATE_VIEWPORT_COUNT,
		MW_DYNAMIC_STATE_SCISSOR_COUNT,
		MW_DYNAMIC_STATE_LINE_WIDTH,
		MW_DYNAMIC_STATE_CULL_MODE,
		MW_DYNAMIC_STATE_FRONT_FACE
	};

	enum ECullMode : u8
	{
		MW_CULL_MODE_NONE,

		MW_CULL_MODE_BACK,
		MW_CULL_MODE_FRONT,
		MW_CULL_MODE_FRONT_AND_BACK,

		MW_CULL_MODE_COUNT
	};

	enum EPolygonMode : u8
	{
		MW_POLYGON_MODE_FILL,
		MW_POLYGON_MODE_LINE,
		MW_POLYGON_MODE_POINT,

		MW_POLYGON_MODE_COUNT
	};

	enum EShaderStage : u8
	{
		MW_SHADER_STAGE_UNKNOWN,
		
		MW_SHADER_STAGE_VERTEX, 
		MW_SHADER_STAGE_TESSELATION_CONTROL, 
		MW_SHADER_STAGE_TESSELATION_EVALUATION, 
		MW_SHADER_STAGE_GEOMETRY,
		MW_SHADER_STAGE_FRAGMENT,
		MW_SHADER_STAGE_COMPUTE, 

		MW_SHADER_STAGE_COUNT
	};

	enum EBlendMode : u8
	{
		MW_BLEND_MODE_NONE,

		MW_BLEND_MODE_OPAQUE,
		MW_BLEND_MODE_APLHA_BLEND,
		MW_BLEND_MODE_ADDITIVE, 
		MW_BLEND_MODE_MULTIPLY,

		MW_BLEND_MODE_COUNT
	};

	enum EPipelineCreationFlagBits 
	{
		MW_PIPELINE_CREATION_FLAGS_NONE_BIT = 0,

		MW_PIPELINE_CREATION_FLAGS_TESSELATION_CONTROL_SHADER_BIT = 0x01,
		MW_PIPELINE_CREATION_FLAGS_TESSELATION_EVALULATION_SHADER_BIT = 0x02,
		MW_PIPELINE_CREATION_FLAGS_GEOMETRY_SHADER_BIT = 0x04,
		MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT = 0x08, 
		MW_PIPELINE_CREATION_FLAGS_DEPTH_CLAMP_BIT = 0x10,
		MW_PIPELINE_CREATION_FLAGS_RASTERIZER_DISCARD_BIT = 0x20,
		MW_PIPELINE_CREATION_FLAGS_DEPTH_BIAS_BIT = 0x40,
		MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_TEST_BIT = 0x80,
		MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_WRITE_BIT = 0x100,

		MW_PIPELINE_CREATION_FLAGS_MAX_ENUM = 0x7FFFFFFF
	};

	using EPipelineCreationFlags = flags_t;

	struct SShaderByteCode
	{
		const void* pCode;
		size_t Size;
	};

	struct SShaderObject 
	{
		SShaderByteCode Code;
		const char* pEntrypoint = "main";
		EShaderStage ShaderStage;
	};

	struct SColorBlendAttachmentState 
	{
		EBlendMode BlendMode;
		bool BlendEnable;
	};

	struct GraphicsPipelineCreationInfo
	{ 
		CVertexLayout VertexLayout;
		std::vector<SShaderObject> ShaderObjects;
		std::vector<EImageFormat> ColorFormats;
		std::vector<SColorBlendAttachmentState> ColorBlendAttachments;
		std::vector<EDynamicState> DynamicStates = { MW_DYNAMIC_STATE_VIEWPORT, MW_DYNAMIC_STATE_SCISSOR };

		// TODO: Implement custom signature for Graphics Pipeline
		void* MW_NULLABLE Signature = nullptr; // VkPipelineLayout / D3D12RootSignature

		EPipelineCreationFlags Flags;
		EImageFormat DepthAttachmentFormat;
		EImageFormat StencilAttachmentFormat;

		u32 ViewportCount = 1;
		u32 ScissorCount = 1;
		ECompareOp CompareOp = MW_COMPARE_OP_LESS;
		ECullMode CullMode = MW_CULL_MODE_BACK;
		EPrimitiveTopology Topology = MW_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		EPolygonMode PolygonMode = MW_POLYGON_MODE_FILL;
	};

	class IGraphicsPipeline 
	{
	public:
		virtual ~IGraphicsPipeline() NOEXCEPT = default;
		
		virtual void Init( const SGraphicsPipelineCreationInfo* pInfo ) = 0;
		virtual void Shutdown() = 0;
		
		virtual void Invalidate( const SGraphicsPipelineCreationInfo* pInfo ) = 0;

		static Ref<IGraphicsPipeline> Create( const SGraphicsPipelineCreationInfo* pInfo ) NOEXCEPT;
	};
}
