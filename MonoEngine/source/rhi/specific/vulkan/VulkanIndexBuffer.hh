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
		CVulkanIndexBuffer( u32 size ) NOEXCEPT;
		CVulkanIndexBuffer( void* pData, u32 size, u32 offset = 0, bool autoUpload = false ) NOEXCEPT;

		~CVulkanIndexBuffer();

		void SetData( void* pData, u32 size, u32 offset = 0 ) NOEXCEPT override;
		void Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT override;

		NODISCARD VkBuffer* GetVulkanBuffer() NOEXCEPT { return &m_IndexBuffer; };
		NODISCARD u32 GetCount() NOEXCEPT override { return m_Count; };

	private:
		VkBuffer m_IndexBuffer = nullptr;
		VmaAllocation m_IndexBufferAllocation = nullptr;

		VkBuffer m_StagingBuffer = nullptr; 
		VmaAllocation m_StagingBufferAllocation = nullptr;

		u32 m_Count = 0;
		u32 m_SizeBytes = 0;
		u32 m_UploadBytes = 0;
		u32 m_Offset = 0;
		u32 m_StagingBufferSize;
	};
}
