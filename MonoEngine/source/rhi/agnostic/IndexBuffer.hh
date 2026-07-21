#pragma once
#include <common/Base.hh>
#include <common/Math.hh>

namespace Monoworks::RHI
{
	using Index = u32;

    class IIndexBuffer
    {
    public:
        virtual ~IIndexBuffer() {};

        virtual void SetData( void* pData, u64 size, u64 offset = 0 ) NOEXCEPT = 0;

        // TODO: Remove VkCommandBuffer on the agnostic side
        virtual void Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT = 0;

        NODISCARD virtual VkBuffer* GetVulkanBuffer() NOEXCEPT = 0;
        NODISCARD virtual u32 GetCount() NOEXCEPT = 0;

        NODISCARD static Ref<IIndexBuffer> Create( u64 size ) NOEXCEPT;
        NODISCARD static Ref<IIndexBuffer> Create( void* pData, u64 size, u64 offset = 0, bool autoupload = false ) NOEXCEPT;
    };
}