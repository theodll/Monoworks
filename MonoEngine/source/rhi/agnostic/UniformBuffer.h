#pragma once
#include <common/Base.hh>

namespace Monoworks::RHI 
{
	class IUniformBuffer
	{
	public:
		virtual ~IUniformBuffer() NOEXCEPT;

		virtual void SetData( void* pData, u64 size, u64 offset = 0 ) NOEXCEPT = 0;
		virtual void Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT = 0;
		
		NODISCARD virtual VkBuffer* GetVulkanBuffer() NOEXCEPT = 0;
		NODISCARD virtual u64 GetSize() NOEXCEPT = 0;

		NODISCARD static Ref<IUniformBuffer> Create( u64 size, u64 offset = 0 );
	};
}
