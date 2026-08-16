#include <mwpch.hh>
#include "GraphicsPipeline.hh"

#include <rhi/specific/vulkan/VulkanGraphicsPipeline.hh>

#include <core/Application.hh>


namespace Monoworks::RHI 
{
	Ref<IGraphicsPipeline> IGraphicsPipeline::Create( const SGraphicsPipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch ( CApplication::GetGraphicsAPI() )
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanGraphicsPipeline>::Create( pInfo );
		}
		MW_ASSERT( false, "Unknown Graphics API" );
		return nullptr;
	};
}
