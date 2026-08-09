#pragma once
#include <common/Base.hh>


#include <volk.h>

#include <vk_mem_alloc.h>

namespace Monoworks 
{
	class CApplication;
}

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
		u32 PresentFamily;
		bool PresentFamilyHasValue = false;
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	class CVulkanDevice 
	{
	public:
		void Init(VkInstance* instance) noexcept;
		void Shutdown() noexcept;

		static VkResult CreateBuffer
		(
			VmaAllocator* pAllocator,
			VkBuffer* pBuffer,
			VmaAllocation* pBufferMemory,
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties
		) NOEXCEPT;

		static VkResult CreateImage
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

		static void CopyImageToBuffer
		(
			VkCommandBuffer* pCmdBuffer,
			VkImage* pSrc,
			VkBuffer* pDst,
			u32 width,
			u32 height,
			u32 layerCount
		) NOEXCEPT;

		VkDevice* GetDevice() noexcept { return &m_Device; };
		const VkPhysicalDevice* GetPhysicalDevice() noexcept { return &m_PhysicalDevice; }

		NODISCARD VkCommandPool* GetGraphicsCommandPool() noexcept { return &m_GraphicsCommandPool; }
		NODISCARD VkQueue* GetGraphicsQueue() noexcept { return &m_GraphicsQueue; }
		
		NODISCARD VkCommandPool* GetComputeCommandPool() noexcept { return &m_ComputeCommandPool; }
		NODISCARD VkQueue* GetComputeQueue() noexcept { return &m_ComputeQueue; }

		NODISCARD VkCommandPool* GetTransferCommandPool() noexcept { return &m_TransferCommandPool; }
		NODISCARD VkQueue* GetTransferQueue() noexcept { return &m_TransferQueue; }

		NODISCARD VkQueue* GetPresentQueue() NOEXCEPT;

		NODISCARD u32 GetGraphicsQueueFamilyIndex() noexcept { return FindQueueFamilies(&m_PhysicalDevice).GraphicsFamily; }

		NODISCARD u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

		NODISCARD QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(&m_PhysicalDevice); }
		
		NODISCARD VkFormat FindSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );

		NODISCARD SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice pPhysDevice, VkSurfaceKHR pSurface );

		void CreatePhysicalDevice(VkInstance* instance) noexcept;

	private:
		void CreateLogicalDevice() noexcept;
		void CreateCommandPool() noexcept;

		NODISCARD bool IsDeviceSuitable( const VkPhysicalDevice* pPhysDevice ) noexcept;
		NODISCARD QueueFamilyIndices FindQueueFamilies( const VkPhysicalDevice* pPhysDevice ) noexcept;
		NODISCARD bool CheckDeviceExtensionSupport( const VkPhysicalDevice* pPhysDevice ) noexcept;

		VkPhysicalDeviceProperties m_Properties;

		VkDevice m_Device = nullptr;
		VkPhysicalDevice m_PhysicalDevice = nullptr;

		VkCommandPool m_GraphicsCommandPool = nullptr;
		VkQueue m_GraphicsQueue = nullptr;

		VkCommandPool m_TransferCommandPool = nullptr;
		VkQueue m_TransferQueue = nullptr;

		VkCommandPool m_ComputeCommandPool = nullptr;
		VkQueue m_ComputeQueue = nullptr;

		VkQueue m_PresentQueue = nullptr;

		VkInstance* m_Instance = nullptr;

		std::vector<const char*> m_DeviceExtensions =
		{
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, 
			VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
#ifdef MW_PLATFORM_OSX
			"VK_KHR_portabillity_subset"
#endif
		};

	};
}

