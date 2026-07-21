#include <mwpch.hh>

#include <core/Application.hh>

#include "UniformBuffer.h"
#include <rhi/specific/vulkan/VulkanUniformBuffer.h>

namespace Monoworks::RHI 
{


	NODISCARD Ref<IUniformBuffer> IUniformBuffer::Create( u64 size, u64 offset /*= 0 */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch ( CApplication::GetGraphicsAPI() )
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanUniformBuffer>::Create( size, false, offset );
		}
		MW_ASSERT( false, "Unknown Graphics API" );
		return nullptr;
	}

}
