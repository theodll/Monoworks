#pragma once
#include <common/Base.hh>

#include <Volk/volk.h>

namespace Monoworks::RHI 
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};


	struct QueueFamilyIndicies
	{
		u32 GraphicsFamily;
		bool GraphicsFamilyHasValue = false;
	};

	class CVulkanDevice 
	{
		void Init() noexcept;
		void Shutdown() noexcept;

		[[nodiscard]] VkResult CreateBuffer
		(
			VkDevice* device,
			VkBuffer* buffer,
			VkDeviceMemory* bufferMemorey,
			VkDeviceSize size,
			VkBufferUsageFlagBits usage,
			VkMemoryPropertyFlags properties
		) noexcept;

		[[nodiscard]] VkResult CopyBuffer
		(
			VkBuffer* pSrc,
			VkBuffer* pDst,
			VkDeviceSize size
		) noexcept;

		[[nodiscard]] VkResult CopyBufferToImage
		(
			VkCommandBuffer* pCommandBuffer,
			VkBuffer* pSrc,
			VkImage* pDst,
			u32 width,
			u32 height,
			u32 layerCount
		) noexcept;

		[[nodiscard]] VkResult CreateImage
		(
			VkImage* pImage,
			const VkImageCreateInfo* pImageInfo,
			VkDeviceMemory* pImageMemory,
			VkMemoryPropertyFlags properties
		) noexcept;

		const VkDevice* GetDevice() const noexcept { return &m_Device; };
		const VkPhysicalDevice* GetPhysicalDevice() const noexcept { return &m_PhysicalDevice; }
		const VkCommandPool* GetCommandPool() const noexcept { return &m_CommandPool; }
		const VkQueue* GetGraphicsQueue() const noexcept { return &m_GraphicsQueue; }
		const u32 GetQueueFamilyIndex() noexcept { return FindQueueFamilies(&m_PhysicalDevice).GraphicsFamily; }

		SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(&m_PhysicalDevice); }
		u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndicies FindPhysicalQueueFamilies() { return FindQueueFamilies(&m_PhysicalDevice); }
		VkFormat FindSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


	private:
		void CreatePhysicalDevice() noexcept;
		void CreateLogicalDevice() noexcept;
		void CreateCommandPool() noexcept;

		[[nodiscard]] bool IsDeviceSuitable(const VkPhysicalDevice* pPhysDevice) const noexcept;
		[[nodiscard]] QueueFamilyIndicies FindQueueFamilies(const VkPhysicalDevice* pPhysDevice) noexcept;
		[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice* pPhysDevice) noexcept;
		[[nodiscard]] bool CheckDeviceExtensionSupport(const VkPhysicalDevice* pPhysDevice) noexcept;

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;

		VkCommandPool m_CommandPool;
		VkQueue m_GraphicsQueue;

		VkInstance* m_Instance;

		const std::vector<const char*> deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef MW_PLATFORM_OSX
			VK_KHR_portability_subset
#endif
		};

	};
}

