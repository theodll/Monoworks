#pragma once
#include <common/Base.hh>

#include <volk/volk.h>
#include <vk_mem_alloc.h>

namespace Monoworks::RHI 
{
	struct QueueFamilyIndices
	{
		u32 GraphicsFamily;
		bool GraphicsFamilyHasValue = false;
		u32 TransferFamily;
		bool TransferFamilyHasValue = false;
		u32 ComputeFamily;
		bool ComputeFamilyHasValue = false; 
	};

	class CVulkanDevice 
	{
	public:
		void Init(VkInstance* instance) noexcept;
		void Shutdown() noexcept;

		NODISCARD static VkResult CreateBuffer
		(
			VmaAllocator* pAllocator,
			VkBuffer* pBuffer,
			VmaAllocation* pBufferMemory,
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties
		) NOEXCEPT;

		NODISCARD static VkResult CreateImage
		(
			VmaAllocator* pAllocator,
			VkImage* pImage,
			const VkImageCreateInfo* pImageInfo,
			VmaAllocation* pImageMemory,
			VkMemoryPropertyFlags properties
		) NOEXCEPT;

		static void CopyBuffer
		(
			VkCommandBuffer* pCmdBuffer,
			VkBuffer* pSrc,
			VkBuffer* pDst,
			VkDeviceSize size
		) NOEXCEPT;

		static void CopyBufferToImage
		(
			VkCommandBuffer* pCmdBuffer,
			VkBuffer* pSrc,
			VkImage* pDst,
			u32 width,
			u32 height,
			u32 layerCount
		) NOEXCEPT;

		const VkDevice* GetDevice() const noexcept { return &m_Device; };
		const VkPhysicalDevice* GetPhysicalDevice() const noexcept { return &m_PhysicalDevice; }

		const VkCommandPool* GetGraphicsCommandPool() const noexcept { return &m_GraphicsCommandPool; }
		const VkQueue* GetGraphicsQueue() const noexcept { return &m_GraphicsQueue; }
		
		const VkCommandPool* GetComputeCommandPool() const noexcept { return &m_ComputeCommandPool; }
		const VkQueue* GetComputeQueue() const noexcept { return &m_ComputeQueue; }

		const VkCommandPool* GetTransferCommandPool() const noexcept { return &m_TransferCommandPool; }
		const VkQueue* GetTransferQueue() const noexcept { return &m_TransferQueue; }

		const u32 GetQueueFamilyIndex() noexcept { return FindQueueFamilies(&m_PhysicalDevice).GraphicsFamily; }

		u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(&m_PhysicalDevice); }
		VkFormat FindSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


	private:
		void CreatePhysicalDevice() noexcept;
		void CreateLogicalDevice() noexcept;
		void CreateCommandPool() noexcept;

		[[nodiscard]] bool IsDeviceSuitable(const VkPhysicalDevice* pPhysDevice) noexcept;
		[[nodiscard]] QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice* pPhysDevice) noexcept;
		[[nodiscard]] bool CheckDeviceExtensionSupport(const VkPhysicalDevice* pPhysDevice) noexcept;

		VkPhysicalDeviceProperties m_Properties;

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;

		VkCommandPool m_GraphicsCommandPool;
		VkQueue m_GraphicsQueue;

		VkCommandPool m_TransferCommandPool;
		VkQueue m_TransferQueue;

		VkCommandPool m_ComputeCommandPool;
		VkQueue m_ComputeQueue;

		VkInstance* m_Instance;

		std::vector<const char*> m_DeviceExtensions =
		{
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, 
			VK_KHR_SHADER_SUBGROUP_ROTATE_EXTENSION_NAME, 
			VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
#ifdef MW_PLATFORM_OSX
			VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
		};

	};
}

