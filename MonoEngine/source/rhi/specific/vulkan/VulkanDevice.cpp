#include <mwpch.hh>
#include <common/Base.hh>

#include <Volk/volk.h>

#include "VulkanDevice.h"

namespace Monoworks::RHI 
{


	void CVulkanDevice::Init() noexcept
	{
		MW_PROFILE_FUNC();
		MW_INFO("Initialize CVulkanDevice");

		CreatePhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
	}

	void CVulkanDevice::Shutdown() noexcept
	{
		MW_PROFILE_FUNC();
		MW_INFO("Shutdown CVulkanDevice");
	}


	u32 CVulkanDevice::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
	{

	}

	VkFormat CVulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{

	}

	void CVulkanDevice::CreatePhysicalDevice() noexcept
	{

	}

	void CVulkanDevice::CreateLogicalDevice() noexcept
	{

	}

	void CVulkanDevice::CreateCommandPool() noexcept
	{

	}

	bool CVulkanDevice::IsDeviceSuitable(const VkPhysicalDevice* pPhysDevice) const noexcept
	{

	}

	QueueFamilyIndicies CVulkanDevice::FindQueueFamilies(const VkPhysicalDevice* pPhysDevice) noexcept
	{

	}

	SwapChainSupportDetails CVulkanDevice::QuerySwapChainSupport(const VkPhysicalDevice* pPhysDevice) noexcept
	{

	}

	bool CVulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice* pPhysDevice) noexcept
	{

	}

	VkResult CVulkanDevice::CreateBuffer(VkDevice* device, VkBuffer* buffer, VkDeviceMemory* bufferMemorey, VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties) noexcept
	{

	}

	VkResult CVulkanDevice::CopyBuffer(VkBuffer* pSrc, VkBuffer* pDst, VkDeviceSize size) noexcept
	{

	}

	VkResult CVulkanDevice::CopyBufferToImage(VkCommandBuffer* pCommandBuffer, VkBuffer* pSrc, VkImage* pDst, u32 width, u32 height, u32 layerCount) noexcept
	{

	}

	VkResult CVulkanDevice::CreateImage(VkImage* pImage, const VkImageCreateInfo* pImageInfo, VkDeviceMemory* pImageMemory, VkMemoryPropertyFlags properties) noexcept
	{

	}

}
