#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/GraphicsContext.hh>

#include <rhi/specific/vulkan/VulkanDevice.h>

#include <Volk/volk.h>

#define WMA_VULKAN_VERSION 1004000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define WMA_DYNAMIC_VULKAN_FUNCTIONS 0 
#include <vk_mem_alloc.h>

namespace Monoworks::RHI
{
	class CVulkanContext : public IGraphicsContext 
	{
	public:
		void Init() override;
		void Shutdown() override;

		static const VkInstance* GetInstance() { return &m_Instance; }
		static const CVulkanDevice* GetDevice() { return &m_Device; }

	private:
		void CreateInstance();
		void CreateVmaAllocator();

		[[nodiscard]] std::vector<const char*> GetRequiredExtensions() noexcept;
	
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo) noexcept;
		void SetupDebugMessenger() noexcept;
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) noexcept;
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) noexcept;

		VkDebugUtilsMessengerEXT m_DebugMessenger;
		static VkInstance m_Instance;
		static VmaAllocator m_Allocator;
		static CVulkanDevice m_Device;
		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		bool m_EnableValidationLayers;

	};
}
