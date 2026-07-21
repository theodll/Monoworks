#include <mwpch.hh>

#include "VulkanContext.hh"

#include "VulkanIndexBuffer.h"

namespace Monoworks::RHI
{

	CVulkanIndexBuffer::CVulkanIndexBuffer( u64 size ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	CVulkanIndexBuffer::CVulkanIndexBuffer( void* pData, u64 size, u64 offset, bool autoUpload ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto allocator = CVulkanContext::GetAllocator();

		CVulkanContext::GetDevice()->CreateBuffer(
			allocator,
			&m_IndexBuffer,
			&m_IndexBufferAllocation,
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		SetData( pData, m_UploadBytes, 0 );


		if ( autoUpload )
		{
			auto uploader = CVulkanContext::GetUploader();

			uploader->Begin();
			Upload( uploader->GetCommandBuffer() );
			uploader->End();
		}
	}

	CVulkanIndexBuffer::~CVulkanIndexBuffer()
	{
		MW_PROFILE_FUNC;

		auto allocator = CVulkanContext::GetAllocator();

		if ( m_StagingBuffer != VK_NULL_HANDLE )
		{
			vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );
		}

		if ( m_IndexBuffer != VK_NULL_HANDLE )
		{
			vmaDestroyBuffer( *allocator, m_IndexBuffer, m_IndexBufferAllocation );
		}
	}


	void CVulkanIndexBuffer::SetData( void* pData, u64 size, u64 offset /*= 0 */ ) NOEXCEPT
	{
		auto allocator = CVulkanContext::GetAllocator();

		if ( m_StagingBuffer == VK_NULL_HANDLE || m_StagingBufferSize < size ) 
		{
			if ( m_StagingBuffer != VK_NULL_HANDLE ) 
			{
				vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );
			}

			CVulkanContext::GetDevice()->CreateBuffer(
				allocator,
				&m_StagingBuffer,
				&m_StagingBufferAllocation,
				size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
			m_StagingBufferSize = size;
		}

		void* mapped = nullptr;
		vmaMapMemory( *allocator, m_StagingBufferAllocation, &mapped );
		memcpy( mapped, pData, ( size_t )size );
		vmaUnmapMemory( *allocator, m_StagingBufferAllocation );
	}

	void CVulkanIndexBuffer::Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_PROFILE_VK_TRANSFER_ZONE(*pCmdBuffer, "Indexbuffer Upload");

		MW_ASSERT( m_StagingBuffer != VK_NULL_HANDLE, "Invalid staging buffer" );

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = m_Offset;
		copyRegion.size = m_UploadBytes;

		vkCmdCopyBuffer(
			*pCmdBuffer,
			m_StagingBuffer,
			m_IndexBuffer,
			1,
			&copyRegion
		);
	}

}
