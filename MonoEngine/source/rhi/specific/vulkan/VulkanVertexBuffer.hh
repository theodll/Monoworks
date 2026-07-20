#pragma once
#include <common/Base.hh>
#include <rhi/agnostic/VertexBuffer.hh>

#include <vk_mem_alloc.h>

namespace Monoworks::RHI 
{
	class VulkanVertexBuffer : public IVertexBuffer
	{
	public:
		VulkanVertexBuffer();
		~VulkanVertexBuffer();

		virtual void SetData(void* data, u64 size, u64 offset = 0) = 0;
		virtual void SetLayout(const CBufferLayout& layout) = 0;

		virtual void Upload(VkCommandBuffer commandBuffer) = 0;

		virtual CBufferLayout GetLayout() const = 0;
		virtual VkBuffer GetVulkanBuffer() const = 0;

	private:
		VkBuffer m_VertexBuffer;
		VmaAllocation m_VertexBufferAllocation;

		VkBuffer m_StagingBuffer;
		VmaAllocation m_StagingBufferAllocation;
	};
}