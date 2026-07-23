#pragma once
#include <common/Base.hh>

#include <volk/volk.h>
#include <vk_mem_alloc.h>

#include <rhi/agnostic/IndexBuffer.hh>

namespace Monoworks::RHI 
{
	class CVulkanIndexBuffer : public IIndexBuffer 
	{
		
	public:
		CVulkanIndexBuffer( u64 size ) NOEXCEPT;
		CVulkanIndexBuffer( void* pData, u64 size, u64 offset = 0, bool autoUpload = false ) NOEXCEPT;

		~CVulkanIndexBuffer();

		void SetData( void* pData, u64 size, u64 offset = 0 ) NOEXCEPT override;
		void Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT override;

		NODISCARD VkBuffer* GetVulkanBuffer() NOEXCEPT override { return &m_IndexBuffer; };
		NODISCARD u32 GetCount() NOEXCEPT override { return m_Count; };

	private:
		VkBuffer m_IndexBuffer;
		VmaAllocation m_IndexBufferAllocation;

		VkBuffer m_StagingBuffer; 
		VmaAllocation m_StagingBufferAllocation;

		u64 m_Count = 0;
		u64 m_SizeBytes = 0;
		u64 m_UploadBytes = 0;
		u64 m_Offset = 0;
		u64 m_StagingBufferSize;
	};
}
