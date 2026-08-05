#include <mwpch.hh>
#include <common/Base.hh>

#include <core/Application.hh>

#include <volk/volk.h>
#include <set>

#include "VulkanDevice.hh"
#include "VulkanContext.hh"

namespace Monoworks::RHI 
{


	void CVulkanDevice::Init(VkInstance* instance) noexcept
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CVulkanDevice" );
		m_Instance = instance;

		if (CApplication::GetCreateInfos()->UseSwapchain)
		{
			m_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		if ( !CApplication::GetCreateInfos()->UseSwapchain )
		{
			// for qt viewport
#ifdef MW_PLATFORM_WINDOWS
			m_DeviceExtensions.push_back( "VK_KHR_external_semaphore_win32" );
			m_DeviceExtensions.push_back( "VK_KHR_external_memory_win32" );
#else
			m_DeviceExtensions.push_back( "VK_KHR_external_semaphore_fd" );
			m_DeviceExtensions.push_back( "VK_KHR_external_memory_fd" ); 
#endif
		}

		CreateLogicalDevice();
		CreateCommandPool();
		
	}

	void CVulkanDevice::Shutdown() noexcept
	{
		MW_PROFILE_FUNC;

		vkDeviceWaitIdle(m_Device);
		// TODO: Allocation Callbacks
		if ( m_GraphicsCommandPool )
			vkDestroyCommandPool(m_Device, m_GraphicsCommandPool, nullptr);

		if ( m_ComputeCommandPool )
			vkDestroyCommandPool( m_Device, m_ComputeCommandPool, nullptr );

		if ( m_TransferCommandPool )
			vkDestroyCommandPool(m_Device, m_TransferCommandPool, nullptr);

		if ( m_Device )
			vkDestroyDevice(m_Device, nullptr);

		MW_INFO( "Shutdown CVulkanDevice" );
	}


	u32 CVulkanDevice::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
	{
		MW_PROFILE_FUNC;
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
		for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		MW_ASSERT(false, "Failed to find suitable memory type");
		return 0;
	}

	VkFormat CVulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		MW_PROFILE_FUNC;
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (
				tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		MW_ASSERT(false, "Failed to find supported format");
		return VK_FORMAT_UNDEFINED;
	}

	void CVulkanDevice::CreatePhysicalDevice(VkInstance* instance) noexcept
	{
		MW_PROFILE_FUNC;
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			MW_ERROR("Failed to find GPUs with Vulkan support");
		}
		MW_INFO("Device count: {}", deviceCount);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(&device))
			{
				m_PhysicalDevice = device;
				break;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			MW_ASSERT(false, "Failed to find a Suitable Physical Device.");
		}

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties); //  <-- hier
		MW_INFO("Physical Device: {}", m_Properties.deviceName);
	}

	void CVulkanDevice::CreateLogicalDevice() noexcept
	{
		MW_PROFILE_FUNC;
		QueueFamilyIndices indices = FindQueueFamilies(&m_PhysicalDevice);

		std::set<u32> uniqueQueueFamilies;

		if ( CApplication::GetCreateInfos()->UseSwapchain )
		{
			uniqueQueueFamilies = { indices.GraphicsFamily, indices.ComputeFamily, indices.TransferFamily, indices.PresentFamily };
		} else 
		{
			uniqueQueueFamilies = { indices.GraphicsFamily, indices.ComputeFamily, indices.TransferFamily };
		}

		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(uniqueQueueFamilies.size());

		std::array<float, 3> priorities = {};
		u32 priorityIndex = 0;

		for (u32 queueFamily : uniqueQueueFamilies)
		{
			float currentPriority = 1.0f;

			if (queueFamily == indices.TransferFamily && queueFamily != indices.GraphicsFamily)
			{
				currentPriority = 0.2f; 
			}
			else if (queueFamily == indices.ComputeFamily && queueFamily != indices.GraphicsFamily)
			{
				currentPriority = 0.5f;
			}
			else if ( queueFamily == indices.PresentFamily && queueFamily != indices.GraphicsFamily )
			{
				currentPriority = 1.0f;
			}

			priorities[priorityIndex] = currentPriority;

			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priorities[priorityIndex];

			queueCreateInfos.push_back(queueCreateInfo);
			priorityIndex++;
		}

		VkPhysicalDeviceVulkan13Features vulkan13Features{};
		vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vulkan13Features.dynamicRendering = VK_TRUE;
		vulkan13Features.synchronization2 = VK_TRUE;
		vulkan13Features.maintenance4 = VK_TRUE;
		vulkan13Features.pNext = nullptr;

		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
		deviceFeatures2.features.geometryShader = VK_TRUE;
		deviceFeatures2.features.tessellationShader = VK_TRUE;
		deviceFeatures2.features.fillModeNonSolid = VK_TRUE;
		deviceFeatures2.features.depthClamp = VK_TRUE;
		deviceFeatures2.features.multiDrawIndirect = VK_TRUE;
		deviceFeatures2.features.drawIndirectFirstInstance = VK_TRUE;
		deviceFeatures2.features.textureCompressionBC = VK_TRUE;
		deviceFeatures2.features.fragmentStoresAndAtomics = VK_TRUE;
		deviceFeatures2.features.vertexPipelineStoresAndAtomics = VK_TRUE;
		deviceFeatures2.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
		deviceFeatures2.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
		deviceFeatures2.features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
		deviceFeatures2.features.independentBlend = VK_TRUE;
		deviceFeatures2.features.multiViewport = VK_TRUE;

		deviceFeatures2.pNext = &vulkan13Features;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = nullptr;
		createInfo.pNext = &deviceFeatures2;

		createInfo.enabledExtensionCount = static_cast<u32>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

		if (!validationLayers.empty())
		{
			createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
		{
			MW_ASSERT(false, "Failed to create logical device");
		}

		vkGetDeviceQueue( m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue );
		vkGetDeviceQueue( m_Device, indices.ComputeFamily, 0, &m_ComputeQueue );
		vkGetDeviceQueue( m_Device, indices.TransferFamily, 0, &m_TransferQueue );
		
		if ( CApplication::GetCreateInfos()->UseSwapchain )
		{
			vkGetDeviceQueue( m_Device, indices.PresentFamily, 0, &m_PresentQueue );
		}

	}

	void CVulkanDevice::CreateCommandPool() noexcept
	{
		MW_PROFILE_FUNC;
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(&m_PhysicalDevice);

		VkCommandPoolCreateInfo graphicsPoolInfo{};
		graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		graphicsPoolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
		graphicsPoolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		MW_VK_CHECK(vkCreateCommandPool(m_Device, &graphicsPoolInfo, nullptr, &m_GraphicsCommandPool), "Failed to create graphics command pool");

		VkCommandPoolCreateInfo computePoolInfo{};
		computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		computePoolInfo.queueFamilyIndex = queueFamilyIndices.ComputeFamily;
		computePoolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		MW_VK_CHECK(vkCreateCommandPool(m_Device, &computePoolInfo, nullptr, &m_ComputeCommandPool), "Failed to create compute command pool");
	

		VkCommandPoolCreateInfo transferPoolInfo{};
		transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		transferPoolInfo.queueFamilyIndex = queueFamilyIndices.TransferFamily;
		transferPoolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		MW_VK_CHECK(vkCreateCommandPool(m_Device, &transferPoolInfo, nullptr, &m_TransferCommandPool), "Failed to create transfer command pool");
	}

	bool CVulkanDevice::IsDeviceSuitable(const VkPhysicalDevice* pPhysDevice) noexcept
	{
		QueueFamilyIndices indices = FindQueueFamilies( pPhysDevice );

		bool extensionsSupported = CheckDeviceExtensionSupport( pPhysDevice );

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(*pPhysDevice, &supportedFeatures);

		VkPhysicalDevicePushDescriptorProperties pushDescriptors{};
		pushDescriptors.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES;

		VkPhysicalDeviceSynchronization2Features sync2Features{};
		sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		sync2Features.synchronization2 = VK_TRUE;
		sync2Features.pNext = &pushDescriptors;

		VkPhysicalDeviceFeatures2 features2 = {};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &sync2Features;

		vkGetPhysicalDeviceFeatures2(*pPhysDevice, &features2);

		if ( CApplication::GetCreateInfos()->UseSwapchain ) 
		{
			return	indices.GraphicsFamilyHasValue &&
				indices.ComputeFamilyHasValue &&
				indices.TransferFamilyHasValue &&
				indices.PresentFamilyHasValue &&
				extensionsSupported &&
				supportedFeatures.samplerAnisotropy;
		}

		return	indices.GraphicsFamilyHasValue &&
			indices.ComputeFamilyHasValue &&
			indices.TransferFamilyHasValue &&
			extensionsSupported &&
			supportedFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices CVulkanDevice::FindQueueFamilies(const VkPhysicalDevice* pPhysDevice) noexcept
	{
		MW_PROFILE_FUNC;
		QueueFamilyIndices indices{};

		u32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( *pPhysDevice, &queueFamilyCount, nullptr );

		std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( *pPhysDevice, &queueFamilyCount, queueFamilies.data() );

		int i = 0;
		for ( const auto& queueFamily : queueFamilies )
		{
			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				indices.GraphicsFamily = i;
				indices.GraphicsFamilyHasValue = true;
			}
			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
			{
				indices.ComputeFamily = i;
				indices.ComputeFamilyHasValue = true;
			}
			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT )
			{
				indices.TransferFamily = i;
				indices.TransferFamilyHasValue = true;
			}

			if ( CApplication::GetCreateInfos()->UseSwapchain && CApplication::GetCreateInfos()->pPresenter->GetMedium() == MW_PRESENTATION_MEDIUM_VULKAN_SDL )
			{
				auto presenter = CVulkanContext::GetPresenter();
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR( *pPhysDevice, i, *( VkSurfaceKHR* )presenter->GetSurface(), &presentSupport );
				if ( queueFamily.queueCount > 0 && presentSupport )
				{
					indices.PresentFamily = i;
					indices.PresentFamilyHasValue = true;
				}
			}

			const bool coreFound = indices.GraphicsFamilyHasValue && indices.ComputeFamilyHasValue && indices.TransferFamilyHasValue;
			const bool presentFound = !CApplication::GetCreateInfos()->UseSwapchain || indices.PresentFamilyHasValue;
			
			if ( coreFound && presentFound )
			{
				break;
			}

			i++;
		}

		return indices;
	}

	bool CVulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice* pPhysDevice) noexcept
	{
		MW_PROFILE_FUNC;
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(*pPhysDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(
			*pPhysDevice,
			nullptr,
			&extensionCount,
			availableExtensions.data());

		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	VkResult CVulkanDevice::CreateBuffer(
		VmaAllocator* pAllocator,
		VkBuffer* pBuffer, 
		VmaAllocation* pBufferMemory,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties) noexcept
	{
		MW_PROFILE_FUNC;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.requiredFlags = properties;

		if ( properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
			allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		
		auto res = vmaCreateBuffer(*pAllocator, &bufferInfo, &allocInfo, pBuffer, pBufferMemory, nullptr);
		MW_VK_CHECK(res, "Failed to create buffer");
		MW_PROFILE_ALLOC_N((void*)pBuffer, size, "GPU VRAM");

		return res;
	}

	VkResult CVulkanDevice::CreateImage(
		VmaAllocator* pAllocator, 
		VkImage* pImage, 
		const VkImageCreateInfo* pImageInfo,
		VmaAllocation* pImageMemory, 
		VkMemoryPropertyFlags properties) noexcept
	{
		MW_PROFILE_FUNC;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.requiredFlags = properties;

		auto res = vmaCreateImage(*pAllocator, pImageInfo, &allocInfo, pImage, pImageMemory, nullptr);
		MW_VK_CHECK(res, "Failed to create buffer");
		MW_PROFILE_ALLOC_N((void*)pImage, pImageInfo->extent.width * pImageInfo->extent.height, "GPU VRAM");

		return res;
	}

	void CVulkanDevice::CopyBuffer(
		VkCommandBuffer* pCmdBuffer,
		VkBuffer* pSrc,
		VkBuffer* pDst,
		VkDeviceSize size) noexcept
	{
		MW_PROFILE_FUNC;

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; 
		copyRegion.dstOffset = 0; 
		copyRegion.size = size;
		vkCmdCopyBuffer(*pCmdBuffer, *pSrc, *pDst, 1, &copyRegion);

	}

	void CVulkanDevice::CopyBufferToImage(
		VkCommandBuffer* pCmdBuffer,
		VkBuffer* pSrc,
		VkImage* pDst,
		u32 width,
		u32 height,
		u32 layerCount) noexcept
	{
		MW_PROFILE_FUNC;

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			*pCmdBuffer,
			*pSrc,
			*pDst,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
	}

	void CVulkanDevice::CopyImageToBuffer(
		VkCommandBuffer* pCmdBuffer,
		VkImage* pSrc,
		VkBuffer* pDst,
		u32 width, 
		u32 height, 
		u32 layerCount ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		VkBufferImageCopy bufferRegion{};
		bufferRegion.bufferOffset = 0;
		bufferRegion.bufferRowLength = 0;
		bufferRegion.bufferImageHeight = 0;
		bufferRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferRegion.imageSubresource.mipLevel = 0;
		bufferRegion.imageSubresource.baseArrayLayer = 0;
		bufferRegion.imageSubresource.layerCount = layerCount;
		bufferRegion.imageOffset = { 0, 0, 0 };
		bufferRegion.imageExtent = { width, height, 1 };

		vkCmdCopyImageToBuffer( *pCmdBuffer, *pSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *pDst, 1, &bufferRegion );

	}

	NODISCARD SwapChainSupportDetails CVulkanDevice::QuerySwapChainSupport( VkPhysicalDevice pPhysDevice, VkSurfaceKHR pSurface )
	{
		MW_PROFILE_FUNC;

		if ( !CApplication::GetCreateInfos()->UseSwapchain )
		{
			MW_ERROR("Illegal function call: Unable to queury swapchain support. UseSwapchain is false");
		}


		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( pPhysDevice, pSurface, &details.Capabilities );  

		u32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR( pPhysDevice, pSurface, &formatCount, nullptr);

		if ( formatCount != 0 )
		{
			details.Formats.resize( formatCount );
			vkGetPhysicalDeviceSurfaceFormatsKHR( pPhysDevice, pSurface, &formatCount, details.Formats.data() );
		}

		u32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR( pPhysDevice, pSurface, &presentModeCount, nullptr );

		if ( presentModeCount != 0 )
		{
			details.PresentModes.resize( presentModeCount );
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				pPhysDevice,
				pSurface,
				&presentModeCount,
				details.PresentModes.data() );
		}
		return details;
	}

	NODISCARD const VkQueue* CVulkanDevice::GetPresentQueue() const NOEXCEPT
	{
		 if ( !CApplication::GetCreateInfos()->UseSwapchain )
		 {
			 MW_ERROR( "Illegal function call: Unable to get Present Queue. UseSwapchain is false" );
			 return nullptr; 
		 }
		 return &m_PresentQueue; 
	}

}
