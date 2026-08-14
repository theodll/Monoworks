#pragma once
#include <common/Base.hh>
#include <rhi/agnostic/VertexBuffer.hh>

#include <vk_mem_alloc.h>

namespace Monoworks::RHI 
{
	class CVulkanVertexBuffer : public IVertexBuffer
	{
	public:
		CVulkanVertexBuffer( void* data, u32 size, u32 offset = 0, bool autoUpload = false ) NOEXCEPT;
		~CVulkanVertexBuffer() NOEXCEPT;

		void SetData( void* data, u32 size, u32 offset = 0 ) NOEXCEPT override;
		void SetLayout( const CBufferLayout& layout ) NOEXCEPT override;

		void Upload( VkCommandBuffer commandBuffer ) NOEXCEPT;

		NODISCARD CBufferLayout*    GetLayout()			NOEXCEPT override { return &m_Layout;  };
		NODISCARD VkBuffer*			GetVulkanBuffer()	NOEXCEPT { return &m_VertexBuffer; };

	private:
		VkBuffer m_VertexBuffer = nullptr;
		VmaAllocation m_VertexBufferAllocation = nullptr;

		VkBuffer m_StagingBuffer = nullptr;
		VmaAllocation m_StagingBufferAllocation = nullptr;

		CBufferLayout m_Layout;

		u32 m_Size				= 0;
		u32 m_UploadSize		= 0;
		u32 m_StagingBufferSize = 0;
		u32 m_Offset			= 0;
	};
}