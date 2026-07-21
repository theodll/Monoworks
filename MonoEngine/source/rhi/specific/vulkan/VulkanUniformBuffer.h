#pragma once
#include <common/Base.hh>

#include <volk/volk.h>
#include <vk_mem_alloc.h>

#include <rhi/agnostic/UniformBuffer.h>

namespace Monoworks::RHI 
{
	class CVulkanUniformBuffer : public IUniformBuffer
	{

	public:
		CVulkanUniformBuffer( u64 size, bool useStaging, u64 offset );
		~CVulkanUniformBuffer();

		void SetData( void* pData, u64 size, u64 offset = 0 ) NOEXCEPT override;
		void Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT override;

		NODISCARD VkBuffer* GetVulkanBuffer() NOEXCEPT override { return &m_UniformBuffer; };
		NODISCARD u64 GetSize() NOEXCEPT override { return m_Size; };
	private:
	
		VkBuffer m_UniformBuffer;
		VmaAllocation m_UniformBufferAllocation;

		VkBuffer m_StagingBuffer;
		VmaAllocation m_StagingBufferAllocation;

		void* m_pMapped;
		bool m_UseStaging;

		u64 m_Size = 0;
		u64 m_UploadSize = 0;
		u64 m_StagingBufferSize = 0;
		u64 m_Offset = 0;
	};
}
