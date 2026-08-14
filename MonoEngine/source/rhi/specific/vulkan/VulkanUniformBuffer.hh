#pragma once
#include <common/Base.hh>

#include <volk/volk.h>
#include <vk_mem_alloc.h>

#include <rhi/agnostic/UniformBuffer.hh>

namespace Monoworks::RHI 
{
	class CVulkanUniformBuffer : public IUniformBuffer
	{

	public:
		CVulkanUniformBuffer( u32 size, bool useStaging, u32 offset );
		~CVulkanUniformBuffer();

		void SetData( void* pData, u32 size, u32 offset = 0 ) NOEXCEPT override;
		void Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT;

		NODISCARD VkBuffer* GetVulkanBuffer() NOEXCEPT { return &m_UniformBuffer; };
		NODISCARD u32 GetSize() NOEXCEPT override { return m_Size; };
	private:
		void DestroyStaging(); 
		void CreateOrResizeStaging( u32 size );


		VkBuffer m_UniformBuffer;
		VmaAllocation m_UniformBufferAllocation;

		VkBuffer m_StagingBuffer;
		VmaAllocation m_StagingBufferAllocation;

		void* m_pMapped;
		bool m_UseStaging;

		u32 m_Size = 0;
		u32 m_UploadSize = 0;
		u32 m_StagingBufferSize = 0;
		u32 m_Offset = 0;
	};
}
