#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/GraphicsContext.hh>
#include <rhi/agnostic/Presenter.hh>

#include <rhi/specific/vulkan/VulkanDevice.h>
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
		void Init() override;
		void Shutdown() override;

		NODISCARD static VkInstance* GetInstance() NOEXCEPT { return &m_Instance; }

		NODISCARD static CVulkanDevice* GetDevice() NOEXCEPT { return &m_Device; }
		NODISCARD static CVulkanResourceUploader* GetUploader() NOEXCEPT { return &m_ResouceUploader; }
		NODISCARD static IPresenter* GetPresenter() NOEXCEPT { return m_Presenter; }

		NODISCARD static VmaAllocator* GetAllocator() NOEXCEPT { return &m_Allocator; }
		

	private:
		void CreateInstance();
		void CreateVmaAllocator();

		NODISCARD std::vector<const char*> GetRequiredExtensions() noexcept;
	
		void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo ) noexcept;
		void SetupDebugMessenger() noexcept;
		VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) noexcept;
		void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) noexcept;

		VkDebugUtilsMessengerEXT m_DebugMessenger;
		static VkInstance m_Instance;
		static VmaAllocator m_Allocator;
		static CVulkanDevice m_Device;
		static CVulkanResourceUploader m_ResouceUploader;
		static IPresenter* m_Presenter;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		bool m_EnableValidationLayers;

	};
}
