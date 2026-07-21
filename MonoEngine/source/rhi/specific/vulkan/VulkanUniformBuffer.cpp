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

		if ( m_UseStaging )
		{
			if ( m_StagingBuffer != nullptr )
			{
				vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );
				m_StagingBufferSize = 0;
			}
		}

		if ( m_UniformBuffer != nullptr )
		{
			vmaDestroyBuffer( *allocator, m_UniformBuffer, m_UniformBufferAllocation );
		}

	}

	void CVulkanUniformBuffer::SetData( void* pData, u64 size, u64 offset /*= 0 */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanUniformBuffer::Upload( VkCommandBuffer* pCmdBuffer ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

}
