#include <mwpch.hh>

#include "VulkanContext.hh"

#include "VulkanUniformBuffer.h"

namespace Monoworks::RHI 
{

	CVulkanUniformBuffer::CVulkanUniformBuffer( u64 size, bool useStaging, u64 offset ) 
		: m_UseStaging( useStaging ), m_Size( size ), m_Offset( offset )
	{
		MW_PROFILE_FUNC;

		auto allocator = CVulkanContext::GetAllocator();

		if ( m_UseStaging )
		{
			CVulkanContext::GetDevice()->CreateBuffer
			(
				allocator,
				&m_UniformBuffer,
				&m_UniformBufferAllocation,
				size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
		} else 
		{
			CVulkanContext::GetDevice()->CreateBuffer
			(
				allocator,
				&m_UniformBuffer,
				&m_UniformBufferAllocation,
				size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		}

		auto res = vmaMapMemory( *allocator, m_UniformBufferAllocation, &m_pMapped );
		MW_ASSERT( res == VK_SUCCESS && m_pMapped != nullptr, "Failed to map uniform buffer memory" );


	}

	CVulkanUniformBuffer::~CVulkanUniformBuffer()
	{
		MW_PROFILE_FUNC;

		auto allocator = CVulkanContext::GetAllocator();

		if ( m_pMapped )
		{
			vmaUnmapMemory(*allocator, m_UniformBufferAllocation);
		}

		if ( m_UniformBuffer != nullptr )
		{
			vmaDestroyBuffer( *allocator, m_UniformBuffer, m_UniformBufferAllocation );
		}

	}

	void CVulkanUniformBuffer::SetData( void* pData, u64 size, u64 offset /*= 0 */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto allocator = CVulkanContext::GetAllocator();

		MW_ASSERT( pData != nullptr, "Uniform Buffer Data is null" );
		MW_ASSERT( size > 0, "Uniform Buffer size is 0" );
		MW_ASSERT( offset + size <= m_Size, "Uniform Buffer write out of bounds" );

		m_UploadSize = size;
		m_Offset = offset;

		const auto& device = CVulkanContext::GetDevice();

		if ( !m_UseStaging )
		{
			memcpy( static_cast<std::byte*>( m_pMapped ) + offset, pData, ( size_t )size );

			return;
		}

		CreateOrResizeStaging( size );

		void* mapped = nullptr;
		VkResult res = vmaMapMemory( *allocator, m_StagingBufferAllocation, &mapped );
		MW_ASSERT( res == VK_SUCCESS && mapped != nullptr, "Failed to map staging buffer memory" );

		memcpy( mapped, pData, ( size_t )(size) );
		vmaUnmapMemory( *allocator, m_StagingBufferAllocation );
	}

	void CVulkanUniformBuffer::Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !m_UseStaging )
			return;

		MW_ASSERT( m_StagingBuffer != VK_NULL_HANDLE, "Staging buffer is invalid" );

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = m_Offset;
		copyRegion.size = m_UploadSize;

		vkCmdCopyBuffer(
			*pCmdBuffer,
			m_StagingBuffer,
			m_UniformBuffer,
			1,
			&copyRegion
		);
	}

	void CVulkanUniformBuffer::DestroyStaging()
	{
		MW_PROFILE_FUNC;
		if ( m_UseStaging )
		{
			if ( m_StagingBuffer != nullptr )
			{
				vmaDestroyBuffer( *CVulkanContext::GetAllocator(), m_StagingBuffer, m_StagingBufferAllocation );
				m_StagingBufferSize = 0;
			}
		}
	}

	void CVulkanUniformBuffer::CreateOrResizeStaging( u64 size )
	{
		MW_PROFILE_FUNC;

		auto alloc = CVulkanContext::GetAllocator();

		if ( m_StagingBuffer == VK_NULL_HANDLE || m_StagingBufferSize < size )
		{
			DestroyStaging();

			CVulkanContext::GetDevice()->CreateBuffer(
				alloc,
				&m_StagingBuffer,
				&m_StagingBufferAllocation,
				size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);

			m_StagingBufferSize = size;
		}
	}

}
