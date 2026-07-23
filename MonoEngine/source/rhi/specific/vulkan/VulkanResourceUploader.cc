#include <mwpch.hh>

#include <volk/volk.h>
#include "VulkanResourceUploader.hh"

#include "VulkanDevice.h"
#include "VulkanContext.hh"


namespace Monoworks::RHI 
{


	void CVulkanResourceUploader::Init() noexcept
	{
		MW_PROFILE_FUNC;
		MW_INFO("Initialize CVulkanResouceUploader");
		
		auto commandPool = CVulkanContext::GetDevice()->GetTransferCommandPool();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = *commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		MW_VK_CHECK(vkAllocateCommandBuffers(
			*CVulkanContext::GetDevice()->GetDevice(),
			&allocInfo,
			&m_Commandbuffer), "Failed to create Transfer Command Buffer")

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		MW_VK_CHECK(vkCreateFence(
			*CVulkanContext::GetDevice()->GetDevice(),
			&fenceInfo,
			nullptr,
			&m_Fence), "Failed to create Transfer Fence")
	}

	void CVulkanResourceUploader::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_TRACE("Shutdown CVulkanResourceUploader");
		if (m_Fence)
			vkDestroyFence(*CVulkanContext::GetDevice()->GetDevice(), m_Fence, nullptr);

		m_Fence = VK_NULL_HANDLE;
		m_Commandbuffer = VK_NULL_HANDLE;
	}

	void CVulkanResourceUploader::Begin() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_VK_CHECK(vkWaitForFences(
			*CVulkanContext::GetDevice()->GetDevice(),
			1,
			&m_Fence,
			VK_TRUE,
			UINT64_MAX
		), "Failed to wait for fences");

		vkResetFences(*CVulkanContext::GetDevice()->GetDevice(), 1, &m_Fence);

		vkResetCommandBuffer(m_Commandbuffer, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(m_Commandbuffer, &beginInfo);
		MW_PROFILE_VK_TRANSFER_ZONE(m_Commandbuffer, "Vulkan resource transfer begin")
	}

	void CVulkanResourceUploader::End() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	
		MW_PROFILE_VK_TRANSFER_COLLECT( m_Commandbuffer );

		vkEndCommandBuffer( m_Commandbuffer );

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_Commandbuffer;

		vkQueueSubmit(
			*CVulkanContext::GetDevice()->GetGraphicsQueue(),
			1,
			&submitInfo,
			m_Fence
		);

		vkWaitForFences(
			*CVulkanContext::GetDevice()->GetDevice(),
			1,
			&m_Fence,
			VK_TRUE,
			UINT64_MAX
		);

	}
}
