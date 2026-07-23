#include <common/Base.hh>
#include <core/Application.hh>

#include "IndexBuffer.hh"
#include <rhi/specific/vulkan/VulkanIndexBuffer.h>

namespace Monoworks::RHI
{
	NODISCARD Ref<IIndexBuffer> IIndexBuffer::Create( u64 size ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch ( CApplication::GetGraphicsAPI() )
		{
		case MW_GAPI_NONE:    return nullptr;
		case MW_GAPI_VULKAN:  return Ref<CVulkanIndexBuffer>::Create(size);
		}
		MW_ASSERT(false, "Unknown Graphics API");
		return nullptr;
	};

	NODISCARD Ref<IIndexBuffer> IIndexBuffer::Create( void* pData, u64 size, u64 offset, bool autoupload ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		switch (CApplication::GetGraphicsAPI())
		{
			case MW_GAPI_NONE:    return nullptr;
			case MW_GAPI_VULKAN:  return Ref<CVulkanIndexBuffer>::Create(pData, size, 0, autoupload);
		}
		MW_ASSERT(false, "Unknown Graphics API");
		return nullptr;
	};
}