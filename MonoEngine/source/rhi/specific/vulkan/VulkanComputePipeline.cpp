#include <mwpch.hh>

#include "VulkanComputePipeline.hh"

#include "VulkanContext.hh"

namespace Monoworks::RHI
{


	CVulkanComputePipeline::CVulkanComputePipeline( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT ) )
		{
			Init( pInfo );
		}

	}


	CVulkanComputePipeline::~CVulkanComputePipeline() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		Shutdown();
	}

	void CVulkanComputePipeline::Init( const ComputePipelineCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;

		Invalidate( pInfo );
	}

	void CVulkanComputePipeline::Shutdown()
	{
		MW_PROFILE_FUNC;

		if ( m_VulkanPipelineLayout )
			vkDestroyPipeline( *CVulkanContext::GetDevice()->GetDevice(), m_VulkanPipeline, CVulkanContext::GetCallbacks() );

		if ( m_VulkanPipeline )
			vkDestroyPipeline( *CVulkanContext::GetDevice()->GetDevice(), m_VulkanPipeline, CVulkanContext::GetCallbacks() );
	}


	void CVulkanComputePipeline::Invalidate( const ComputePipelineCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;

		if ( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEPTH_BIAS_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEPTH_CLAMP_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_TEST_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_WRITE_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_GEOMETRY_SHADER_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_TESSELATION_CONTROL_SHADER_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_TESSELATION_EVALULATION_SHADER_BIT
			|| pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_RASTERIZER_DISCARD_BIT )
		{
			MW_API_WARN( "Passes Graphics-Pipeline only creation flags to Compute-Pipeline creation" );
		}

		if ( !pInfo->Signature )
		{

			// TODO: Do descriptor sets & shader reflection 


			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.pushConstantRangeCount = 0;

			vkCreatePipelineLayout( *CVulkanContext::GetDevice()->GetDevice(), &pipelineLayoutInfo, CVulkanContext::GetCallbacks(), &m_VulkanPipelineLayout);
		} 
		else
		{
			m_VulkanPipelineLayout = static_cast< VkPipelineLayout >( pInfo->Signature );
		}

		VkPipelineShaderStageCreateInfo computeShaderStage;
		VkShaderModule computeModule;

		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = pInfo->ComputeShader.Code.Size;
			createInfo.pCode = ( u32* )pInfo->ComputeShader.Code.pCode;

			MW_VK_CHECK( vkCreateShaderModule( *CVulkanContext::GetDevice()->GetDevice(), &createInfo, CVulkanContext::GetCallbacks(), &computeModule ), "Failed to create Shader Module" );
		}

		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.module = computeModule;
		info.pName = pInfo->ComputeShader.pEntrypoint;

		VkComputePipelineCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		createInfo.stage = computeShaderStage;
		createInfo.flags =
			VK_PIPELINE_CREATE_DERIVATIVE_BIT |
			VK_PIPELINE_CREATE_DISPATCH_BASE_BIT |
			VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT;
		createInfo.layout = m_VulkanPipelineLayout;
		
		VkResult res = vkCreateComputePipelines(
			*CVulkanContext::GetDevice()->GetDevice(),
			*CVulkanContext::GetPipelineCache(),
			1,
			&createInfo,
			CVulkanContext::GetCallbacks(),
			&m_VulkanPipeline
		);

		if ( res < 0 )
		{
			MW_ERROR( "Non-Fataly failed to create compute pipeline.");
		}

	}

}
