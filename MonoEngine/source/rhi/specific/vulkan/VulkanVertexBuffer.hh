#pragma once
#include <common/Base.hh>
#include <rhi/agnostic/VertexBuffer.hh>

#include <vk_mem_alloc.h>

namespace Monoworks::RHI 
{
	class CVulkanVertexBuffer : public IVertexBuffer
	{
	public:
		CVulkanVertexBuffer( void* data, u64 size, u64 offset = 0, bool autoUpload = false ) NOEXCEPT;
		~CVulkanVertexBuffer() NOEXCEPT;

		void SetData( void* data, u64 size, u64 offset = 0 ) NOEXCEPT;
		void SetLayout( const CBufferLayout& layout ) NOEXCEPT;

		void Upload(VkCommandBuffer commandBuffer) NOEXCEPT;

		NODISCARD CBufferLayout*    GetLayout()			NOEXCEPT override { return &m_Layout;  };
		NODISCARD VkBuffer*			GetVulkanBuffer()	NOEXCEPT override { return &m_VertexBuffer; };

	private:
		VkBuffer m_VertexBuffer;
		VmaAllocation m_VertexBufferAllocation;

		VkBuffer m_StagingBuffer;
		VmaAllocation m_StagingBufferAllocation;

		CBufferLayout m_Layout;

		u64 m_Size				= 0;
		u64 m_UploadSize		= 0;
		u64 m_StagingBufferSize = 0;
		u64 m_Offset			= 0;
	};
}