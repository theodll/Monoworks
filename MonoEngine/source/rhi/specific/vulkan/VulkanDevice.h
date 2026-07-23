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

		const VkDevice* GetDevice() const noexcept { return &m_Device; };
		const VkPhysicalDevice* GetPhysicalDevice() const noexcept { return &m_PhysicalDevice; }

		NODISCARD const VkCommandPool* GetGraphicsCommandPool() const noexcept { return &m_GraphicsCommandPool; }
		NODISCARD const VkQueue* GetGraphicsQueue() const noexcept { return &m_GraphicsQueue; }
		
		NODISCARD const VkCommandPool* GetComputeCommandPool() const noexcept { return &m_ComputeCommandPool; }
		NODISCARD const VkQueue* GetComputeQueue() const noexcept { return &m_ComputeQueue; }

		NODISCARD const VkCommandPool* GetTransferCommandPool() const noexcept { return &m_TransferCommandPool; }
		NODISCARD const VkQueue* GetTransferQueue() const noexcept { return &m_TransferQueue; }

		NODISCARD const VkQueue* GetPresentQueue() const NOEXCEPT { if ( !CApplication::GetCreateInfos()->UseSwapchain ) { MW_ERROR( "Illegal function call: Unable to get Present Queue. UseSwapchain is false" ); return nullptr; } return &m_PresentQueue; }

		NODISCARD const u32 GetGraphicsQueueFamilyIndex() noexcept { return FindQueueFamilies(&m_PhysicalDevice).GraphicsFamily; }

		NODISCARD u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

		NODISCARD QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(&m_PhysicalDevice); }
		
		NODISCARD VkFormat FindSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );

		NODISCARD SwapChainSupportDetails QuerySwapChainSupport( const VkPhysicalDevice* pPhysDevice, VkSurfaceKHR* pSurface );

		void CreatePhysicalDevice(VkInstance* instance) noexcept;

	private:
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

		VkQueue m_PresentQueue;

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

