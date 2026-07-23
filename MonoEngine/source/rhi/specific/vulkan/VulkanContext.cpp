#include <mwpch.hh>

#include "VulkanContext.hh"
#include "VulkanDevice.h"

#include "VulkanPresenter.hh"

#include <core/Application.hh>
#include <events/EventManager.hh>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1004000
#include <vk_mem_alloc.h>

#ifdef MW_PROFILING
#include <tracy/Tracy.hpp>

TracyVkCtx TracyGraphicsContext = nullptr;
TracyVkCtx TracyComputeContext	= nullptr;
TracyVkCtx TracyTransferContext = nullptr;

#endif

namespace Monoworks::RHI 
{

	CVulkanDevice CVulkanContext::m_Device;
	IPresenter* CVulkanContext::m_Presenter;
	 
	VmaAllocator CVulkanContext::m_Allocator;
	VkInstance CVulkanContext::m_Instance;
	CVulkanResourceUploader CVulkanContext::m_ResouceUploader;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		MW_PROFILE_FUNC;

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			MW_TRACE("Vulkan: {}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			MW_INFO("Vulkan: {}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			MW_WARN("Vulkan: {}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			MW_ERROR("Vulkan: {}", pCallbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}

	void CVulkanContext::Init()
	{
		MW_PROFILE_FUNC;
		MW_INFO("Initialize CVulkanContext");

		MW_VK_CHECK(volkInitialize(), "Failed to Initialize Volk");

		CreateInstance();

		volkLoadInstance(m_Instance);
		
		SetupDebugMessenger();
		m_Device.CreatePhysicalDevice(&m_Instance);
		
		SVulkanSDLPresentationInitializationInfo presentationInfo;

		m_Presenter->Init( &presentationInfo );

		m_Device.Init(&m_Instance);
		volkLoadDevice(*m_Device.GetDevice());

		SVulkanSDLPresentationInitialization2Info presentationInfo2;
		m_Presenter->Init2( &presentationInfo2 );

		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.physicalDevice = *m_Device.GetPhysicalDevice();
		allocatorCreateInfo.device = *m_Device.GetDevice();
		allocatorCreateInfo.instance = m_Instance;
		allocatorCreateInfo.vulkanApiVersion = MW_VK_VERSION;
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT
			| VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT
			| VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

		VmaVulkanFunctions vulkanFunctions;
		MW_VK_CHECK(vmaImportVulkanFunctionsFromVolk(&allocatorCreateInfo, &vulkanFunctions), "Failed to import vulkan functions from volk for VMA");

		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		MW_VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator), "Failed to create VMA Allocator");

		m_ResouceUploader.Init(); 


#ifdef MW_PROFILING
		VmaTotalStatistics stats;
		vmaCalculateStatistics(m_Allocator, &stats);

		CEventManager::Subscribe(MW_EVENT_APP_FRAME, [&] (SEvent& event )
			{
				MW_PROFILE_PLOT("VRAM Total Allocated", (s64)stats.total.statistics.blockBytes);
				MW_PROFILE_PLOT("VRAM Usage", (s64)stats.total.statistics.allocationBytes);
				MW_PROFILE_PLOT("Total GPU Allocations", (s64)stats.total.statistics.allocationCount);
				return false;
			});

		m_ResouceUploader.Begin();
		TracyGraphicsContext = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetGraphicsQueue(), *m_ResouceUploader.GetCommandBuffer() );
		TracyComputeContext	 = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetComputeQueue(), *m_ResouceUploader.GetCommandBuffer() );
		TracyTransferContext = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetTransferQueue(), *m_ResouceUploader.GetCommandBuffer() );
		m_ResouceUploader.End();

#endif


	}

	static inline bool CheckValidationLayerSupport( const std::vector<const char*>& validationLayers )
	{
		MW_PROFILE_FUNC;

		u32 layerCount;
		vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

		std::vector<VkLayerProperties> availableLayers( layerCount );
		vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp( layerName, layerProperties.layerName ) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;

	}

	void CVulkanContext::Shutdown()
	{
		MW_PROFILE_FUNC;
		MW_INFO("Shutdown CVulkanContext");
	}

	void CVulkanContext::CreateInstance()
	{
		MW_PROFILE_FUNC;
		if (m_EnableValidationLayers && !CheckValidationLayerSupport(m_ValidationLayers))
		{
			MW_ERROR("Validation layers requested, but not available!");
		}

		auto ApplicationInfos = CApplication::GetCreateInfos();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = ApplicationInfos->Name;
		appInfo.applicationVersion = VK_MAKE_VERSION(ApplicationInfos->Version.Major, ApplicationInfos->Version.Minor, ApplicationInfos->Version.Patch);
		appInfo.pEngineName = EngineName;
		appInfo.engineVersion = VK_MAKE_VERSION(MonoworksVersion.Major, MonoworksVersion.Minor, MonoworksVersion.Patch);
		appInfo.apiVersion = MW_VK_VERSION;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
#ifdef MW_PLATFORM_OSX
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = (u32)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = DebugCallback;
			debugCreateInfo.pUserData = nullptr;

			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}
		
		MW_VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance), "Failed to create Vulkan Instance");

		u32 extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> appExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, appExtensions.data());

		std::unordered_set<std::string> available;
		for (const auto& extension : appExtensions)
		{
			available.insert(extension.extensionName);
		}

		auto requiredExtensions = GetRequiredExtensions();
		for (const auto& required : requiredExtensions)
		{
			if (available.find(required) == available.end())
			{
				MW_ERROR("Missing required App extension: {}", required);
			}
		}
	

	}

	void CVulkanContext::CreateVmaAllocator()
	{
		MW_PROFILE_FUNC;
		VmaVulkanFunctions vulkanFunctions{};


	}

	void CVulkanContext::SetupDebugMessenger() noexcept
	{
		MW_PROFILE_FUNC;
		if (!m_EnableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = DebugCallback;
		debugCreateInfo.pUserData = nullptr;

		MW_VK_CHECK(CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger), "Failed to setup debug messenger");
	}


	std::vector<const char*> CVulkanContext::GetRequiredExtensions() noexcept
	{
		MW_PROFILE_FUNC;

		u32 extensionCount2 = 0;

		auto appDetails = CApplication::GetCreateInfos();

		const char** clientExtensions = nullptr;

		if (appDetails->RequiredExtensionCallback)
		{

			clientExtensions = appDetails->RequiredExtensionCallback(&extensionCount2);
		}

		std::vector<const char*> requiredExtensions;
		if (clientExtensions != nullptr && extensionCount2 > 0)
		{
			requiredExtensions.assign(clientExtensions, clientExtensions + extensionCount2);
		}

#ifdef MW_PLATFORM_OSX
		extensions.push_back("VK_KHR_portability_enumeration");
#endif

		requiredExtensions.push_back("VK_KHR_get_physical_device_properties2");

		if (m_EnableValidationLayers)
		{
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		
		return requiredExtensions;

	}
	

	VkResult CVulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) noexcept
	{
		MW_PROFILE_FUNC;
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance,
			"vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void CVulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) noexcept
	{

	}

	void CVulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo) noexcept
	{

	}

}
