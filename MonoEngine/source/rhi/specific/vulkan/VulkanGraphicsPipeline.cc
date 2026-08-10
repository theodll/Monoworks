#include <mwpch.hh>
#include "VulkanGraphicsPipeline.hh"

#include <rhi/agnostic/GraphicsPipeline.hh>

#include <rhi/specific/vulkan/VulkanContext.hh>

namespace Monoworks::RHI 
{


	CVulkanGraphicsPipeline::CVulkanGraphicsPipeline( const SPipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT ) )
		{
			Init( pInfo );
		}
	}

	CVulkanGraphicsPipeline::~CVulkanGraphicsPipeline() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		Shutdown();
	}

	void CVulkanGraphicsPipeline::Init( const SPipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_VertexLayout = pInfo->VertexLayout;
		Invalidate( pInfo );
	}

	void CVulkanGraphicsPipeline::Shutdown()
	{
		MW_PROFILE_FUNC; 

		auto device = CVulkanContext::GetDevice()->GetDevice();

		vkDestroyPipeline( *device, m_VulkanPipeline, CVulkanContext::GetCallbacks() );
		vkDestroyPipelineLayout( *device, m_VulkanPipelineLayout, CVulkanContext::GetCallbacks() );
	}

	static VkFormat ShaderDataTypeToVulkanFormat( EShaderDataType type )
	{
		MW_PROFILE_FUNC;
		switch ( type )
		{
		case MW_SHADER_DATA_TYPE_FLOAT:  return VK_FORMAT_R32_SFLOAT;
		case MW_SHADER_DATA_TYPE_FLOAT_2: return VK_FORMAT_R32G32_SFLOAT;
		case MW_SHADER_DATA_TYPE_FLOAT_3: return VK_FORMAT_R32G32B32_SFLOAT;
		case MW_SHADER_DATA_TYPE_FLOAT_4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case MW_SHADER_DATA_TYPE_INT:    return VK_FORMAT_R32_SINT;
		case MW_SHADER_DATA_TYPE_INT_2:   return VK_FORMAT_R32G32_SINT;
		case MW_SHADER_DATA_TYPE_INT_3:   return VK_FORMAT_R32G32B32_SINT;
		case MW_SHADER_DATA_TYPE_INT_4:   return VK_FORMAT_R32G32B32A32_SINT;
		case MW_SHADER_DATA_TYPE_BOOL:   return VK_FORMAT_R8_UINT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	static VkPrimitiveTopology ToVulkanPrimitiveTopology( EPrimitiveTopology topology )
	{
		MW_PROFILE_FUNC;
		switch ( topology )
		{
		case MW_PRIMITIVE_TOPOLOGY_POINT_LIST:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case MW_PRIMITIVE_TOPOLOGY_LINE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case MW_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			MW_API_ERROR("Pass invalid EPrimitiveTopology Enumeration.");
			return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
		}
	}

	static VkCullModeFlagBits ToVulkanCullMode( ECullMode mode )
	{
		MW_PROFILE_FUNC;
		switch ( mode )
		{
		case MW_CULL_MODE_NONE:
			return VK_CULL_MODE_NONE;
		case MW_CULL_MODE_FRONT:
			return VK_CULL_MODE_FRONT_BIT;
		case MW_CULL_MODE_BACK:
			return VK_CULL_MODE_BACK_BIT;
		case MW_CULL_MODE_FRONT_AND_BACK:
			return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			MW_API_ERROR( "Pass invalid ECullMode Enumeration." );
			return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
		}

	}

	static VkPolygonMode ToVulkanPolygonMode( EPolygonMode mode )
	{
		MW_PROFILE_FUNC;
		switch ( mode )
		{
		case MW_POLYGON_MODE_FILL:
			return VK_POLYGON_MODE_FILL;
		case MW_POLYGON_MODE_LINE:
			return VK_POLYGON_MODE_LINE;
		case MW_POLYGON_MODE_POINT:
			return VK_POLYGON_MODE_POINT;
		default:
			MW_API_ERROR( "Pass invalid EPolygonMode Enumeration." );
			return VK_POLYGON_MODE_MAX_ENUM;
		}
	}

	static VkCompareOp ToVulkanCompareOp( ECompareOp compareOp )
	{
		MW_PROFILE_FUNC;
		switch ( compareOp )
		{
		case MW_COMPARE_OP_NEVER:
			return VK_COMPARE_OP_NEVER;
		case MW_COMPARE_OP_LESS:
			return VK_COMPARE_OP_LESS;
		case MW_COMPARE_OP_EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case MW_COMPARE_OP_LESS_OR_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case MW_COMPARE_OP_GREATER:
			return VK_COMPARE_OP_GREATER;
		case MW_COMPARE_OP_NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
		case MW_COMPARE_OP_GREATER_OR_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case MW_COMPARE_OP_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			MW_API_ERROR( "Pass invalid ECompareOp Enumeration." );
			return VK_COMPARE_OP_MAX_ENUM;
		}
	}

	static VkShaderStageFlagBits ToVulkanShaderStage( EShaderStage stage, EPipelineFlags flags )
	{
		switch ( stage )
		{
		case MW_SHADER_STAGE_VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case MW_SHADER_STAGE_FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case MW_SHADER_STAGE_TESSELATION_CONTROL:
		{
			if ( !( flags & MW_PIPELINE_CREATION_FLAGS_TESSELATION_CONTROL_SHADER_BIT ) )
			{
				MW_API_ERROR( "Pass Tesselation Control Shader without MW_PIPELINE_CREATION_FLAGS_TESSELATION_CONTROL_SHADER_BIT set." );
			}
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		}
		case MW_SHADER_STAGE_TESSELATION_EVALUATION:
		{
			if ( !( flags & MW_PIPELINE_CREATION_FLAGS_TESSELATION_EVALULATION_SHADER_BIT ) )
			{
				MW_API_ERROR( "Pass Tesselation Evaluation Shader without MW_PIPELINE_CREATION_FLAGS_TESSELATION_EVALULATION_SHADER_BIT set." );
			}
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		}
		case MW_SHADER_STAGE_GEOMETRY:
		{
			if ( !( flags & MW_PIPELINE_CREATION_FLAGS_GEOMETRY_SHADER_BIT ) )
			{
				MW_API_ERROR( "Pass Geometry Shader without MW_PIPELINE_CREATION_FLAGS_GEOMETRY_SHADER_BIT set." );
			}
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		}
		case MW_SHADER_STAGE_COMPUTE:
		{
			MW_API_ERROR( "Pass Compute Shader to Graphics Pipeline" );
			return VK_SHADER_STAGE_COMPUTE_BIT;
		}
		default:
		{
			MW_API_ERROR( "Pass invalid EShaderStage Enumeration." );
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}
		};
	};

	void CVulkanGraphicsPipeline::Invalidate( const SPipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice()->GetDevice();
		MW_TRACE( "Create Vulkan Pipeline" );

		struct alignas( 16 ) PushConstantData
		{
			Vector4 Color;
		};

		// TODO: Do descriptor sets & shader reflection 

		VkPushConstantRange range{};
		range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		range.offset = 0;
		range.size = sizeof( PushConstantData );

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &range;

		vkCreatePipelineLayout( *device, &pipelineLayoutInfo, CVulkanContext::GetCallbacks(), &m_VulkanPipelineLayout );

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		std::vector<VkShaderModule> modules;

		for ( const auto& object : pInfo->ShaderObjects )
		{
			VkShaderModule module;

			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = object.Code.Size;
			createInfo.pCode = ( u32* )object.Code.pCode;

			MW_VK_CHECK( vkCreateShaderModule( *CVulkanContext::GetDevice()->GetDevice(), &createInfo, CVulkanContext::GetCallbacks(), &module ), "Failed to create Shader Module" );

			modules.push_back( module );

			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.module = module;
			info.pName = object.pEntrypoint;
			

			info.stage = ToVulkanShaderStage(object.ShaderStage, pInfo->Flags);

			shaderStages.push_back( info );
		}

		for ( const auto blendAttachment : pInfo->ColorBlendAttachments )
		{
			MW_PROFILE_FUNC;
			VkPipelineColorBlendAttachmentState attachment{};
			attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

			switch ( blendAttachment.BlendMode ) {
			case MW_BLEND_MODE_OPAQUE:
				attachment.blendEnable = VK_FALSE;
				break;

			case MW_BLEND_MODE_APLHA_BLEND:
				attachment.blendEnable = VK_TRUE;
				attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				attachment.colorBlendOp = VK_BLEND_OP_ADD;
				attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp = VK_BLEND_OP_ADD;
				break;

			case MW_BLEND_MODE_ADDITIVE:
				attachment.blendEnable = VK_TRUE;
				attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
				attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp = VK_BLEND_OP_ADD;
				attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				attachment.alphaBlendOp = VK_BLEND_OP_ADD;
				break;

			case MW_BLEND_MODE_MULTIPLY:
				attachment.blendEnable = VK_TRUE;
				attachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
				attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
				attachment.colorBlendOp = VK_BLEND_OP_ADD;
				attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
				attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp = VK_BLEND_OP_ADD;
				break;
			}

			m_ColorAttachmentStates.push_back(attachment);
		}

		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.stride = m_VertexLayout.GetStride();
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attributeDescs;
		uint32_t location = 0;
		for ( const auto& element : m_VertexLayout.GetElements() )
		{
			if ( element.Type == MW_SHADER_DATA_TYPE_MAT_3 || element.Type == MW_SHADER_DATA_TYPE_MAT_4 )
			{
				uint32_t count = ( element.Type == MW_SHADER_DATA_TYPE_MAT_4 ) ? 4 : 3;
				const bool isMat4 = ( element.Type == MW_SHADER_DATA_TYPE_MAT_4 );
				const VkFormat columnFormat = isMat4 ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R32G32B32_SFLOAT;
				const uint32_t columnStride = isMat4 ? 16u : 12u;
				for ( uint32_t i = 0; i < count; i++ )
				{
					VkVertexInputAttributeDescription attr{};
					attr.binding = 0;
					attr.location = location++;
					attr.format = columnFormat;
					attr.offset = element.Offset + ( i * columnStride );
					attributeDescs.push_back( attr );
				}
			}
			else
			{
				VkVertexInputAttributeDescription attr{};
				attr.binding = 0;
				attr.location = location++;
				attr.format = ShaderDataTypeToVulkanFormat( element.Type );
				attr.offset = element.Offset;
				attributeDescs.push_back( attr );
			}
		}

		for ( const auto dynamicState : pInfo->DynamicStates )
		{
			switch ( dynamicState )
			{
			case MW_DYNAMIC_STATE_SCISSOR_COUNT: 
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT );
				break;
			case MW_DYNAMIC_STATE_VIEWPORT_COUNT:
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT );
				break;
			case MW_DYNAMIC_STATE_SCISSOR:
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR );
				break;
			case MW_DYNAMIC_STATE_VIEWPORT:
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT );
				break;
			case MW_DYNAMIC_STATE_CULL_MODE:
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_CULL_MODE );
				break;
			case MW_DYNAMIC_STATE_FRONT_FACE:
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_FRONT_FACE );
				break;
			case MW_DYNAMIC_STATE_LINE_WIDTH:
				m_DynamicStates.push_back( VK_DYNAMIC_STATE_LINE_WIDTH );
				break;
			default:
				MW_API_ERROR("Pass invalid EDynamicState Enum");
				break;

			}
		}

		VkPipelineVertexInputStateCreateInfo pipelineVertexInputInfo{};
		pipelineVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineVertexInputInfo.vertexBindingDescriptionCount = 1;
		pipelineVertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
		pipelineVertexInputInfo.vertexAttributeDescriptionCount = ( u32 )attributeDescs.size();
		pipelineVertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
		pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		pipelineRenderingCreateInfo.colorAttachmentCount = ( u32 )pInfo->ColorFormats.size();
		pipelineRenderingCreateInfo.pColorAttachmentFormats = ( VkFormat* )pInfo->ColorFormats.data();
		pipelineRenderingCreateInfo.depthAttachmentFormat = ( VkFormat )pInfo->DepthAttachmentFormat;
		pipelineRenderingCreateInfo.stencilAttachmentFormat = ( VkFormat )pInfo->StencilAttachmentFormat;

		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyCreateInfo{};
		pipelineInputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyCreateInfo.topology = ToVulkanPrimitiveTopology( pInfo->Topology );

		VkPipelineViewportStateCreateInfo pipelineViewportCreateInfo{};
		pipelineViewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineViewportCreateInfo.scissorCount = pInfo->ScissorCount;
		pipelineViewportCreateInfo.viewportCount = pInfo->ViewportCount;
		pipelineViewportCreateInfo.pScissors = nullptr; // dynamic in cmd buffer
		pipelineViewportCreateInfo.pViewports = nullptr; // dynamic in cmd buffer too

		// TODO: Add tesselation 

		VkPipelineRasterizationStateCreateInfo pipelineRasterizationCreateInfo{};
		pipelineRasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineRasterizationCreateInfo.rasterizerDiscardEnable = pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_RASTERIZER_DISCARD_BIT;
		pipelineRasterizationCreateInfo.depthClampEnable = pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEPTH_CLAMP_BIT;
		pipelineRasterizationCreateInfo.depthBiasClamp = pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEPTH_BIAS_BIT;
		pipelineRasterizationCreateInfo.cullMode = ToVulkanCullMode( pInfo->CullMode );
		pipelineRasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pipelineRasterizationCreateInfo.polygonMode = ToVulkanPolygonMode( pInfo->PolygonMode );
		pipelineRasterizationCreateInfo.lineWidth = 1.0f;
		// TODO: add values for depth bias  

		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
		pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		// TODO: add multisampling

		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
		pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineDepthStencilStateCreateInfo.depthTestEnable = !( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_TEST_BIT );
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = !( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_WRITE_BIT );
		pipelineDepthStencilStateCreateInfo.depthCompareOp = ToVulkanCompareOp( pInfo->CompareOp ); 

		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
		pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineColorBlendStateCreateInfo.attachmentCount = ( u32 )m_ColorAttachmentStates.size();
		pipelineColorBlendStateCreateInfo.pAttachments = m_ColorAttachmentStates.data();

		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
		pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineDynamicStateCreateInfo.dynamicStateCount = ( u32 )m_DynamicStates.size();
		pipelineDynamicStateCreateInfo.pDynamicStates = m_DynamicStates.data();

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
		graphicsPipelineCreateInfo.stageCount = ( u32 )shaderStages.size();
		graphicsPipelineCreateInfo.layout = m_VulkanPipelineLayout;
		graphicsPipelineCreateInfo.pStages = shaderStages.data();
		graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &pipelineViewportCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
		graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

		if ( vkCreateGraphicsPipelines( *device, *CVulkanContext::GetPipelineCache(), 1, &graphicsPipelineCreateInfo, CVulkanContext::GetCallbacks(), &m_VulkanPipeline ) != VK_SUCCESS )
		{
			MW_ERROR("Non-Fataly failed to create some graphics pipelines");
		};

		for ( const auto& module : modules )
		{
			vkDestroyShaderModule( *device, module, CVulkanContext::GetCallbacks() );
		}

	};

}
