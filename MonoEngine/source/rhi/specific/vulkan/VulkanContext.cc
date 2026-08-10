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
	constexpr size_t g_AllocationWarnLimit = 0x1000000;

	CVulkanDevice CVulkanContext::m_Device;
	IPresenter* CVulkanContext::m_Presenter;
	 
	VmaAllocator CVulkanContext::m_Allocator;
	VkInstance CVulkanContext::m_Instance;
	VkPipelineCache CVulkanContext::m_PipelineCache;
	CVulkanResourceUploader CVulkanContext::m_ResourceUploader;
	VkAllocationCallbacks CVulkanContext::m_AllocationCallbacks;

#ifdef MW_PROFILING
	STotalAllocs CVulkanContext::m_TotalVulkanAllocated{};
#endif


	static void* AlignedAlloc( size_t size, size_t alignment )
	{
		MW_PROFILE_FUNC;

		void* pPtr = nullptr;
#ifdef MW_PLATFORM_WINDOWS
		pPtr = _aligned_malloc( size, alignment );
#else
		constexpr auto alignUp = []( size_t value, size_t alignment ) constexpr { return ( value + ( alignment - 1 ) ) & ~( alignment - 1 ); };
		pPtr = std::aligned_alloc( alignment, alignUp( size, alignment ) );
#endif

		MW_PROFILE_ALLOC( pPtr, size );
		return pPtr;

	}

	static void AlignedFree( void* pBlock ) 
	{
		MW_PROFILE_FUNC;

		if ( !pBlock )
		{
			MW_WARN( "Requested to free an already free block." );
			return;
		}

		MW_PROFILE_FREE( pBlock );

#ifdef MW_PLATFORM_WINDOWS
		_aligned_free( pBlock );
#else
		std::free( pBlock );
#endif

		pBlock = nullptr;

	}


	void* VKAPI_ATTR VkAllocate( void*, size_t size, size_t alignment, VkSystemAllocationScope scope )
	{
		MW_PROFILE_FUNC;
		if ( size == 0 )
			return nullptr;

		constexpr auto alignUp = []( size_t value, size_t alignment ) constexpr { return ( value + ( alignment - 1 ) ) & ~( alignment - 1 ); };

		alignment = std::max<size_t>( alignment, alignof( std::max_align_t ) );
		const size_t headerPad = alignUp( sizeof(SAllocHeader), alignment );

		void* pRaw = AlignedAlloc( headerPad + size, alignment  );
		if ( !pRaw )
		{
			MW_ERROR( "Vulkan Allocation of size {} failed.", size );
			return nullptr;
		}

		if ( size > g_AllocationWarnLimit )
			MW_WARN( "Large Vulkan Allocation: Allocation at {} exceeding 16 Mebibytes: {} Bytes.", ( void* )pRaw, size );


		byte_t* pUser = static_cast< byte_t* >( pRaw ) + headerPad;
		auto* pHeader = reinterpret_cast< SAllocHeader* >( pUser - sizeof( SAllocHeader ) );
		pHeader->pRawBlock = pRaw;
		pHeader->Size = size;
		pHeader->Scope = scope;

#ifdef MW_PROFILING
		auto& total = CVulkanContext::GetTotalVulkanAllocations();

		switch ( scope )
		{
		case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
		{
			total.CommandAllocs += size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
		{
			total.ObjectAllocs += size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
		{
			total.CacheAllocs += size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
		{
			total.DeviceAllocs += size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
		{
			total.InstanceAllocs += size;
			break;
		}
		}
#endif
		return pUser;

	}

	void VKAPI_ATTR VkFree( void*, void* pMemory )
	{
		if ( !pMemory )
			return;

		auto* pHeader = reinterpret_cast< SAllocHeader* >( static_cast< byte_t* >( pMemory ) - sizeof( SAllocHeader ) );

#ifdef MW_PROFILING
		auto& total = CVulkanContext::GetTotalVulkanAllocations();

		switch ( pHeader->Scope )
		{
		case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
		{
			total.CommandAllocs -= pHeader->Size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
		{
			total.ObjectAllocs -= pHeader->Size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
		{
			total.CacheAllocs -= pHeader->Size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
		{
			total.DeviceAllocs -= pHeader->Size;
			break;
		}
		case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
		{
			total.InstanceAllocs -= pHeader->Size;
			break;
		}
		}
#endif

		AlignedFree( pHeader->pRawBlock );

	}

	void* VKAPI_ATTR VkReallocate( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope )
	{
		if ( !pOriginal )
			return VkAllocate( pUserData, size, alignment, scope );

		if ( size == 0 )
		{
			VkFree( pUserData, pOriginal );
			return nullptr;
		}

		auto* pOldHeader = reinterpret_cast< SAllocHeader* >( static_cast< byte_t* >( pOriginal ) - sizeof( SAllocHeader ) );
		const size_t oldSize = pOldHeader->Size;

		void* pNew = VkAllocate( pUserData, size, alignment, scope );
		if ( !pNew )
			return nullptr;

		memcpy( pNew, pOriginal, ( ( ( oldSize ) < ( size ) ) ? ( oldSize ) : ( size ) ) );
		VkFree( pUserData, pOriginal );
		return pNew;
	}


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

		m_AllocationCallbacks.pUserData = nullptr;
		m_AllocationCallbacks.pfnAllocation = &VkAllocate;
		m_AllocationCallbacks.pfnFree = &VkFree;
		m_AllocationCallbacks.pfnReallocation = &VkReallocate;

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
		allocatorCreateInfo.pAllocationCallbacks = &m_AllocationCallbacks;
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT
			| VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT
			| VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

		VmaVulkanFunctions vulkanFunctions;
		MW_VK_CHECK( vmaImportVulkanFunctionsFromVolk( &allocatorCreateInfo, &vulkanFunctions ), "Failed to import vulkan functions from volk for VMA" );

		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

#ifdef MW_PLATFORM_WINDOWS
		vulkanFunctions.vkGetMemoryWin32HandleKHR = vkGetMemoryWin32HandleKHR;
#endif

#ifdef MW_DEBUG
		MW_TRACE( "VMA Function Pointers: " );
		MW_TRACE( "PFN_vkGetInstanceProcAddr: {}",					( void* )vulkanFunctions.vkGetInstanceProcAddr );
		MW_TRACE( "PFN_vkGetDeviceProcAddr: {}",					( void* )vulkanFunctions.vkGetDeviceProcAddr );
		MW_TRACE( "PFN_vkGetPhysicalDeviceProperties: {}",			( void* )vulkanFunctions.vkGetPhysicalDeviceProperties );
		MW_TRACE( "PFN_vkGetPhysicalDeviceMemoryProperties: {}",	( void* )vulkanFunctions.vkGetPhysicalDeviceMemoryProperties );
		MW_TRACE( "PFN_vkAllocateMemory: {}",						( void* )vulkanFunctions.vkAllocateMemory );
		MW_TRACE( "PFN_vkFreeMemory: {}",							( void* )vulkanFunctions.vkFreeMemory );
		MW_TRACE( "PFN_vkMapMemory: {}",							( void* )vulkanFunctions.vkMapMemory );
		MW_TRACE( "PFN_vkUnmapMemory: {}",							( void* )vulkanFunctions.vkUnmapMemory );
		MW_TRACE( "PFN_vkFlushMappedMemoryRanges: {}",				( void* )vulkanFunctions.vkFlushMappedMemoryRanges );
		MW_TRACE( "PFN_vkInvalidateMappedMemoryRanges: {}",			( void* )vulkanFunctions.vkInvalidateMappedMemoryRanges );
		MW_TRACE( "PFN_vkBindBufferMemory: {}",						( void* )vulkanFunctions.vkBindBufferMemory );
		MW_TRACE( "PFN_vkGetBufferMemoryRequirements: {}",			( void* )vulkanFunctions.vkGetBufferMemoryRequirements );
		MW_TRACE( "PFN_vkGetImageMemoryRequirements: {}",			( void* )vulkanFunctions.vkGetImageMemoryRequirements );
		MW_TRACE( "PFN_vkCreateBuffer: {}",							( void* )vulkanFunctions.vkCreateBuffer );
		MW_TRACE( "PFN_vkDestroyBuffer: {}",						( void* )vulkanFunctions.vkDestroyBuffer );
		MW_TRACE( "PFN_vkCreateImage: {}",							( void* )vulkanFunctions.vkCreateImage );
		MW_TRACE( "PFN_vkDestroyImage: {}",							( void* )vulkanFunctions.vkDestroyImage );
		MW_TRACE( "PFN_vkCmdCopyBuffer: {}",						( void* )vulkanFunctions.vkCmdCopyBuffer );
		MW_TRACE( "PFN_vkGetMemoryWin32HandleKHR: {}",				( void* )vulkanFunctions.vkGetMemoryWin32HandleKHR );
#endif

		MW_VK_CHECK( vmaCreateAllocator( &allocatorCreateInfo, &m_Allocator ), "Failed to create VMA Allocator" );


		m_ResourceUploader.Init();

		CVulkanRenderManager::Init();

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

			VkSemaphore* qtReadFinishedSemaphores[MFIF];
			for ( u32 i{}; i < MFIF; i++ )
			{
				qtReadFinishedSemaphores[i] = CVulkanRenderManager::GetQtReadFinishedSemaphore( i );
			}

			SVulkanQtPresentationInitialization2Info presentationInfo2;
			presentationInfo2.pRenderFinishedSemaphores = renderFinishedSemaphores;
			presentationInfo2.RenderFinishedSemaphoreCount = MFIF;
			presentationInfo2.pQtReadFinishedSemaphores = qtReadFinishedSemaphores;
			presentationInfo2.QtReadFinishedSemaphoreCount = MFIF;
			presentationInfo2.pVulkanDevice = &m_Device;

			m_Presenter->Init2( &presentationInfo2 );
		}



#ifdef MW_PROFILING

		m_ResourceUploader.Begin();
		m_ResourceUploader.End();
		
		CEventManager::Subscribe(MW_EVENT_APP_FRAME, +[] (SEvent& event )
			{
				VmaTotalStatistics stats;
				vmaCalculateStatistics( m_Allocator, &stats );
				MW_PROFILE_PLOT("VRAM Total Allocated", (s64)stats.total.statistics.blockBytes);
				MW_PROFILE_PLOT("VRAM Usage", (s64)stats.total.statistics.allocationBytes);
				MW_PROFILE_PLOT("Total GPU Allocations", (s64)stats.total.statistics.allocationCount);
				return false;
			});

		TracyGraphicsContext = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetGraphicsQueue(), *m_ResourceUploader.GetCommandBuffer() );
		TracyComputeContext	 = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetComputeQueue(), *m_ResourceUploader.GetCommandBuffer() );
		TracyTransferContext = MW_PROFILE_VK_CREATE_CTX( *m_Device.GetPhysicalDevice(), *m_Device.GetDevice(), *m_Device.GetTransferQueue(), *m_ResourceUploader.GetCommandBuffer() );

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
		

		MW_PROFILE_VK_DESTROY_CTX( TracyGraphicsContext );
		MW_PROFILE_VK_DESTROY_CTX( TracyComputeContext );
		MW_PROFILE_VK_DESTROY_CTX( TracyTransferContext );

		if ( m_PipelineCache )
		{
			vkDestroyPipelineCache( *m_Device.GetDevice(), m_PipelineCache, CVulkanContext::GetCallbacks() );
		}

		if ( m_Allocator )
		{
			vmaDestroyAllocator( m_Allocator );
		}

		m_ResourceUploader.Shutdown();
		m_Presenter->Shutdown();
		
		m_Device.Shutdown();

		vkDestroyDebugUtilsMessengerEXT( m_Instance, m_DebugMessenger, CVulkanContext::GetCallbacks() );

		if ( m_Instance )
		{
			vkDestroyInstance( m_Instance, CVulkanContext::GetCallbacks() );
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
		VkResult res = vkCreateInstance( &createInfo, &m_AllocationCallbacks, &m_Instance );

		MW_VK_CHECK(res, "Failed to create Vulkan Instance");

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

		MW_VK_CHECK( CreateDebugUtilsMessengerEXT( m_Instance, &debugCreateInfo, &m_AllocationCallbacks, &m_DebugMessenger ), "Failed to setup debug messenger" );
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
