#include <mwpch.hh>

#include <core/Application.hh>

#include "ComputePipeline.hh"
#include <rhi/specific/vulkan/VulkanComputePipeline.hh>

namespace Monoworks::RHI 
{

	Ref<IComputePipeline> IComputePipeline::Create( const ComputePipelineCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;

		switch ( CApplication::GetGraphicsAPI() )
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanComputePipeline>::Create( pInfo );
		}
		MW_ASSERT( false, "Unknown Graphics API" );
		return nullptr;
	}

}
