#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/GraphicsContext.hh>
#include <rhi/agnostic/Presenter.hh>

#include <rhi/specific/vulkan/VulkanDevice.hh>
#include <rhi/specific/vulkan/VulkanResourceUploader.hh>

#include <volk/volk.h>

#ifndef VMA_STATIC_VULKAN_FUNCTIONS
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#endif
#include <vk_mem_alloc.h>

namespace Monoworks::RHI
{
	struct SAllocHeader
	{
		void* pRawBlock;
		size_t Size;
		VkSystemAllocationScope Scope;
	};
#ifdef MW_PROFILING
	struct STotalAllocs 
	{
		// TODO: Make thread safe
		size_t	CommandAllocs; 
		size_t	ObjectAllocs;
		size_t	CacheAllocs;
		size_t	DeviceAllocs;
		size_t	InstanceAllocs;
	};
#endif 

	class CVulkanContext : public IGraphicsContext 
	{
	public:
		void Init() NOEXCEPT override;
		void Shutdown() NOEXCEPT override;

		NODISCARD static VkInstance* GetInstance() NOEXCEPT { return &m_Instance; }
		NODISCARD static VkPipelineCache* GetPipelineCache() NOEXCEPT { return &m_PipelineCache; }

		NODISCARD static CVulkanDevice* GetDevice() NOEXCEPT { return &m_Device; }
		NODISCARD static CVulkanResourceUploader* GetUploader() NOEXCEPT { return &m_ResourceUploader; }
		NODISCARD static IPresenter* GetPresenter() NOEXCEPT { return m_Presenter; }

		NODISCARD static VmaAllocator* GetAllocator() NOEXCEPT { return &m_Allocator; }
		NODISCARD static VkAllocationCallbacks* GetCallbacks() NOEXCEPT { return &m_AllocationCallbacks; }
#ifdef MW_PROFILING
		NODISCARD static STotalAllocs& GetTotalVulkanAllocations() NOEXCEPT { return m_TotalVulkanAllocated; }
#endif

	private:
		void CreateInstance() NOEXCEPT;
		void CreateVmaAllocator() NOEXCEPT;

		NODISCARD std::vector<const char*> GetRequiredExtensions() NOEXCEPT;
	
		void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo ) NOEXCEPT;
		void SetupDebugMessenger() NOEXCEPT;
		VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) NOEXCEPT;
		void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) NOEXCEPT;

		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
#ifdef MW_PROFILING
		static STotalAllocs m_TotalVulkanAllocated;
#endif

		static VkInstance m_Instance;
		static VkPipelineCache m_PipelineCache;
		static VkAllocationCallbacks m_AllocationCallbacks;

		static VmaAllocator m_Allocator;
		
		static CVulkanDevice m_Device;
		static CVulkanResourceUploader m_ResourceUploader;
		
		static IPresenter* m_Presenter;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		bool m_EnableValidationLayers = true;

	};
}
