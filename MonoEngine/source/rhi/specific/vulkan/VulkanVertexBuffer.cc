#include <common/Base.hh>
#include "VulkanVertexBuffer.hh"

#include <rhi/specific/vulkan/VulkanContext.hh>

#include <volk/volk.h>

namespace Monoworks::RHI
{
	CVulkanVertexBuffer::CVulkanVertexBuffer( void* data, u64 size, u64 offset, bool autoupload ) NOEXCEPT
		: m_UploadSize(size), m_Size(size)
	{
		MW_PROFILE_FUNC;


		CVulkanContext::GetDevice()->CreateBuffer(
			CVulkanContext::GetAllocator(),
			&m_VertexBuffer,
			&m_VertexBufferAllocation,
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		SetData(data, size, offset);

		if (autoupload)
		{
			CVulkanContext::GetUploader()->Begin();
			Upload(*CVulkanContext::GetUploader()->GetCommandBuffer());
			CVulkanContext::GetUploader()->End();
		}
	}

	CVulkanVertexBuffer::~CVulkanVertexBuffer() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto allocator = CVulkanContext::GetAllocator();

		if ( m_StagingBuffer != VK_NULL_HANDLE )
		{
			vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );
		}

		if ( m_VertexBuffer != VK_NULL_HANDLE )
		{
			vmaDestroyBuffer( *allocator, m_VertexBuffer, m_VertexBufferAllocation );
		}
	}

	void CVulkanVertexBuffer::SetData( void* data, u64 size, u64 offset ) NOEXCEPT
	{
		auto allocator = CVulkanContext::GetAllocator();

		if ( m_StagingBuffer == VK_NULL_HANDLE || m_StagingBufferSize < size ) {
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

		vmaMapMemory( *CVulkanContext::GetAllocator(), m_StagingBufferAllocation, &mapped );
		memcpy( mapped, data, (size_t)size );
		vmaUnmapMemory( *CVulkanContext::GetAllocator(), m_StagingBufferAllocation );
	}

	void CVulkanVertexBuffer::Upload( VkCommandBuffer commandBuffer ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Upload vertex buffer to GPU" );
		MW_ASSERT( m_StagingBuffer != VK_NULL_HANDLE, "Invalid staging buffer" );

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = m_Offset;
		copyRegion.size = m_UploadSize;

		vkCmdCopyBuffer(
			commandBuffer,
			m_StagingBuffer,
			m_VertexBuffer,
			1,
			&copyRegion
		);
	}


}
