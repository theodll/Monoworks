#include <mwpch.hh>
#include <common/Base.hh>

#include <Volk/volk.h>
#include <set>

#include "VulkanDevice.h"

namespace Monoworks::RHI 
{


	void CVulkanDevice::Init() noexcept
	{
		MW_PROFILE_FUNC();
		MW_INFO("Initialize CVulkanDevice");

		CreatePhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
	}

	void CVulkanDevice::Shutdown() noexcept
	{
		MW_PROFILE_FUNC();
		MW_INFO("Shutdown CVulkanDevice");

		vkDeviceWaitIdle(m_Device);
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		vkDestroyDevice(m_Device, nullptr);
	}


	u32 CVulkanDevice::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
	{
		MW_PROFILE_FUNC();
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
		MW_ERROR("Failed to find suitable memory type");
		__debugbreak();
	}

	VkFormat CVulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		MW_PROFILE_FUNC();
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
		MW_ERROR("Failed to find supported format");
		__debugbreak();
		return VK_FORMAT_UNDEFINED;
	}

	void CVulkanDevice::CreatePhysicalDevice() noexcept
	{
		MW_PROFILE_FUNC();
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(*m_Instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			MW_ERROR("Failed to find GPUs with Vulkan support!");
		}
		MW_INFO("Device count: {}", deviceCount);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(*m_Instance, &deviceCount, devices.data());

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
			MW_ERROR("Failed to find a Suitable Physical Device.");
		}

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
		MW_INFO("Physical Device: {}", m_Properties.deviceName);
	}

	void CVulkanDevice::CreateLogicalDevice() noexcept
	{
		MW_PROFILE_FUNC();
		QueueFamilyIndices indices = FindQueueFamilies(&m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<u32> uniqueQueueFamilies = { indices.GraphicsFamily };

		float queuePriority = 1.0f;
		for (u32 queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceVulkan14Features vulkan14Features{};
		vulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		vulkan14Features.pushDescriptor = true; 
		vulkan14Features.shaderSubgroupRotate = true;
		vulkan14Features.shaderSubgroupRotateClustered = true;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
		deviceFeatures2.pNext = &vulkan14Features;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = nullptr;
		createInfo.pNext = &deviceFeatures2;

		createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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
			MW_ERROR("Failed to create logical device");
			__debugbreak();
		}

		vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue);
	}

	void CVulkanDevice::CreateCommandPool() noexcept
	{
		MW_PROFILE_FUNC();
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(&m_PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
		poolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		{
			MW_ERROR("Failed to create command pool");
			__debugbreak();
		}
	}

	bool CVulkanDevice::IsDeviceSuitable(const VkPhysicalDevice* pPhysDevice) noexcept
	{
		QueueFamilyIndices indices = FindQueueFamilies(pPhysDevice);

		bool extensionsSupported = CheckDeviceExtensionSupport(pPhysDevice);

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(*pPhysDevice, &supportedFeatures);

		VkPhysicalDevicePushDescriptorProperties pushDescriptors{};
		pushDescriptors.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES;

		VkPhysicalDeviceFeatures2 features2 = {};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &pushDescriptors;

		vkGetPhysicalDeviceFeatures2(*pPhysDevice, &features2);

		return indices.GraphicsFamilyHasValue &&
			extensionsSupported &&
			supportedFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices CVulkanDevice::FindQueueFamilies(const VkPhysicalDevice* pPhysDevice) noexcept
	{
		MW_PROFILE_FUNC();
		QueueFamilyIndices indices;

		u32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(*pPhysDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(*pPhysDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.GraphicsFamily = i;
				indices.GraphicsFamilyHasValue = true;
			}
			if (indices.GraphicsFamilyHasValue)
			{
				break;
			}

			i++;
		}

		return indices;
	}

	bool CVulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice* pPhysDevice) noexcept
	{
		MW_PROFILE_FUNC();
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(*pPhysDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(
			*pPhysDevice,
			nullptr,
			&extensionCount,
			availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	VkResult CVulkanDevice::CreateBuffer(VkDevice* device, VkBuffer* buffer, VkDeviceMemory* bufferMemorey, VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties) noexcept
	{
		return VK_SUCCESS;
	}

	VkResult CVulkanDevice::CopyBuffer(VkBuffer* pSrc, VkBuffer* pDst, VkDeviceSize size) noexcept
	{
		return VK_SUCCESS;
	}

	VkResult CVulkanDevice::CopyBufferToImage(VkCommandBuffer* pCommandBuffer, VkBuffer* pSrc, VkImage* pDst, u32 width, u32 height, u32 layerCount) noexcept
	{
		return VK_SUCCESS;
	}

	VkResult CVulkanDevice::CreateImage(VkImage* pImage, const VkImageCreateInfo* pImageInfo, VkDeviceMemory* pImageMemory, VkMemoryPropertyFlags properties) noexcept
	{
		return VK_SUCCESS;
	}

}
