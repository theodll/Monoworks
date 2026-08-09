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
		

	private:
		void CreateInstance() NOEXCEPT;
		void CreateVmaAllocator() NOEXCEPT;

		NODISCARD std::vector<const char*> GetRequiredExtensions() NOEXCEPT;
	
		void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo ) NOEXCEPT;
		void SetupDebugMessenger() NOEXCEPT;
		VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) NOEXCEPT;
		void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) NOEXCEPT;

		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
		static VkInstance m_Instance;
		static VkPipelineCache m_PipelineCache;

		static VmaAllocator m_Allocator;
		
		static CVulkanDevice m_Device;
		static CVulkanResourceUploader m_ResourceUploader;
		
		static IPresenter* m_Presenter;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		bool m_EnableValidationLayers = true;

	};
}
