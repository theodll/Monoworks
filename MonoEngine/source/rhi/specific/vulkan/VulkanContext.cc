#include <mwpch.hh>

#include "VulkanContext.hh"
#include "VulkanDevice.hh"
#include "VulkanRenderManager.hh"

#include "VulkanPresenter.hh"

#include <core/Application.hh>
#include <events/EventManager.hh>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000

#if MW_PLATFORM_WINDOWS
#define VMA_EXTERNAL_MEMORY_WIN32 1
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
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
	VkPipelineCache CVulkanContext::m_PipelineCache;
	CVulkanResourceUploader CVulkanContext::m_ResouceUploader;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData )
	{
		MW_PROFILE_FUNC;

		auto messageTypeFunc = [&]()
			{
				switch ( messageType ) 
				{
				case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
					return "";
				case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
					return "Validation";
				case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
					return "Performance";
				case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
					return "Device AB";
				default:
					return "";
				}
			};

		switch  ( messageSeverity )
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			MW_TRACE( "Vulkan {}: {}", messageTypeFunc(), pCallbackData->pMessage );
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			MW_INFO( "Vulkan {}: {}", messageTypeFunc(), pCallbackData->pMessage );
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			MW_WARN( "Vulkan {}: {}", messageTypeFunc(), pCallbackData->pMessage );
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			MW_ERROR( "Vulkan {}: {}", messageTypeFunc(), pCallbackData->pMessage );
			break;
		default:
			MW_ERROR( "Vulkan {}: {}", messageTypeFunc(), pCallbackData->pMessage );
		}

		return VK_FALSE;
	}

	static SVersion GetVulkanVersion( u32 vulkanVersion = MW_VK_VERSION ) NOEXCEPT
	{
		SVersion temp {};

		temp.Major = static_cast< uint8_t >((vulkanVersion >> 22U) & 0x7FU);
						
		temp.Minor = static_cast< uint8_t >((vulkanVersion >> 12U) & 0x3FFU);
	
		temp.Patch = static_cast< uint16_t >(vulkanVersion & 0xFFFU);

		return temp;
	}

	void CVulkanContext::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CVulkanContext" );

		auto v = GetVulkanVersion();
		MW_INFO( "Vulkan Version {}.{}.{}", v.Major, v.Minor, v.Patch );

		MW_VK_CHECK(volkInitialize(), "Failed to Initialize Volk");

		CreateInstance();

		volkLoadInstance(m_Instance);

		SetupDebugMessenger();
		m_Presenter = CApplication::GetCreateInfos()->pPresenter;

		// Todo: somehow decide which presenter to use 

		SVulkanSDLPresentationSurfaceCreationInfo surfaceInfo{};
		surfaceInfo.pInstance = &m_Instance;
		m_Presenter->CreateSurface( &surfaceInfo );
		
		m_Device.CreatePhysicalDevice(&m_Instance);
		
		SVulkanSDLPresentationInitializationInfo presentationInfo;
		presentationInfo.pInstance = &m_Instance;
		presentationInfo.pDevice = m_Device.GetDevice();
		presentationInfo.pPhysDevice = m_Device.GetPhysicalDevice();
		presentationInfo.pVulkanDevice = &m_Device;
		m_Presenter->Init( &presentationInfo );

		m_Device.Init(&m_Instance);
		volkLoadDevice(*m_Device.GetDevice());

		VmaAllocatorCreateInfo allocatorCreateInfo {};
		allocatorCreateInfo.physicalDevice = *m_Device.GetPhysicalDevice();
		allocatorCreateInfo.device = *m_Device.GetDevice();
		allocatorCreateInfo.instance = m_Instance;
		allocatorCreateInfo.vulkanApiVersion = MW_VK_VERSION;
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT
			| VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT
			| VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

		VmaVulkanFunctions vulkanFunctions;
		MW_VK_CHECK( vmaImportVulkanFunctionsFromVolk( &allocatorCreateInfo, &vulkanFunctions ), "Failed to import vulkan functions from volk for VMA" );

		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		MW_VK_CHECK( vmaCreateAllocator( &allocatorCreateInfo, &m_Allocator ), "Failed to create VMA Allocator" );

		m_ResouceUploader.Init();

		if ( CApplication::GetCreateInfos()->UseSDL )
		{
			SVulkanSDLPresentationInitialization2Info presentationInfo2;
			presentationInfo2.pVulkanDevice = &m_Device;
			presentationInfo2.pDevice = m_Device.GetDevice();
			presentationInfo2.pPhysDevice = *m_Device.GetPhysicalDevice();
			m_Presenter->Init2( &presentationInfo2 );
		}
		else if ( CApplication::GetCreateInfos()->UseQt )
		{
			VkSemaphore* renderFinishedSemaphores[MFIF];
			for ( u32 i {}; i < MFIF; i++ )
			{
				renderFinishedSemaphores[i] = CVulkanRenderManager::GetRenderFinishedSemaphore( i );
			};

			SVulkanQtPresentationInitialization2Info presentationInfo2;
			presentationInfo2.pRenderFinishedSemaphores = renderFinishedSemaphores;
			presentationInfo2.RenderFinishedSemaphoreCount = MFIF;
			presentationInfo2.pVulkanDevice = &m_Device;

			m_Presenter->Init2( &presentationInfo2 );
		}



#ifdef MW_PROFILING

		CEventManager::Subscribe(MW_EVENT_APP_FRAME, +[] (SEvent& event )
			{
				VmaTotalStatistics stats;
				vmaCalculateStatistics( m_Allocator, &stats );
				MW_PROFILE_PLOT("VRAM Total Allocated", (s64)stats.total.statistics.blockBytes);
				MW_PROFILE_PLOT("VRAM Usage", (s64)stats.total.statistics.allocationBytes);
				MW_PROFILE_PLOT("Total GPU Allocations", (s64)stats.total.statistics.allocationCount);
				return false;
			});

		TracyGraphicsContext = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetGraphicsQueue(), *m_ResouceUploader.GetCommandBuffer() );
		TracyComputeContext	 = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetComputeQueue(), *m_ResouceUploader.GetCommandBuffer() );
		TracyTransferContext = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetTransferQueue(), *m_ResouceUploader.GetCommandBuffer() );

#endif
	}

	static inline bool CheckValidationLayerSupport( const std::vector<const char*>& validationLayers ) NOEXCEPT
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

	void CVulkanContext::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		
		// TODO: Allocation Callbacks

		MW_PROFILE_VK_DESTROY_CTX( TracyGraphicsContext );
		MW_PROFILE_VK_DESTROY_CTX( TracyComputeContext );
		MW_PROFILE_VK_DESTROY_CTX( TracyTransferContext );

		if ( m_PipelineCache )
		{
			vkDestroyPipelineCache( *m_Device.GetDevice(), m_PipelineCache, nullptr );
		}

		if ( m_Allocator )
		{
			vmaDestroyAllocator( m_Allocator );
		}

		m_ResouceUploader.Shutdown();
		m_Presenter->Shutdown();
		
		m_Device.Shutdown();

		// TODO: Allocation Callbacks
		vkDestroyDebugUtilsMessengerEXT( m_Instance, m_DebugMessenger, nullptr );

		if ( m_Instance )
		{
			vkDestroyInstance( m_Instance, nullptr );
		}

		MW_INFO( "Shutdown CVulkanContext" );
	}

	void CVulkanContext::CreateInstance() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if (m_EnableValidationLayers && !CheckValidationLayerSupport(m_ValidationLayers))
		{
			MW_ERROR("Validation layers requested, but not available!");
		}

		auto ApplicationInfos = CApplication::GetCreateInfos();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = ApplicationInfos->pName;
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
			debugCreateInfo.messageType = 
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

	void CVulkanContext::CreateVmaAllocator() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		VmaVulkanFunctions vulkanFunctions{};


	}

	void CVulkanContext::SetupDebugMessenger() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if (!m_EnableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = DebugCallback;
		debugCreateInfo.pUserData = nullptr;

		MW_VK_CHECK( CreateDebugUtilsMessengerEXT( m_Instance, &debugCreateInfo, nullptr, &m_DebugMessenger ), "Failed to setup debug messenger" );
	}


	std::vector<const char*> CVulkanContext::GetRequiredExtensions() NOEXCEPT
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
		if (clientExtensions && extensionCount2 > 0)
		{
			requiredExtensions.assign(clientExtensions, clientExtensions + extensionCount2);
		}

#ifdef MW_PLATFORM_OSX
		requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

		requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		if (m_EnableValidationLayers)
		{
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		
		return requiredExtensions;

	}
	

	VkResult CVulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) NOEXCEPT
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

	void CVulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) NOEXCEPT
	{

	}

	void CVulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& pCreateInfo) NOEXCEPT
	{

	}

}
