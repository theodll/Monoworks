#include <Monoworks.hh>

#include "VulkanSDLPresenter.hh"

#include <rhi/specific/vulkan/VulkanPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>

#include <volk/volk.h>
#include <SDL3/SDL_vulkan.h>

namespace Monoworks::RHI 
{
	static void TransitionImageLayout3(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask )
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		if ( oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = 0;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
			newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = 0;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		else
		{
			MW_ASSERT( false, "Unsupported layout transition" );
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	CVulkanSDLPresenter::CVulkanSDLPresenter( SExtent2D swapchainExtent, bool vsync, SDL_Window* window ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_Window = window;

		CEventManager::Subscribe( MW_EVENT_WINDOW_RESIZE, [this]( SEvent& event ) { return this->OnResize( event ); } );

		m_PresentationMedium = MW_PRESENTATION_MEDIUM_VULKAN_SDL;

		m_SwapchainExtent = swapchainExtent;

		m_ImagePresentedOnce.resize(MFIF);

		m_VSync = vsync;

		SDL_SetWindowSize( window, m_SwapchainExtent.Width, m_SwapchainExtent.Height );
	}

	void CVulkanSDLPresenter::Init( const IPresentationInitializationInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CVulkanSDLPresenter" );

		MW_ASSERT(pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium");

		auto info = ( SVulkanSDLPresentationInitializationInfo* )pInfo;
		
		u32 queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties( *info->pPhysDevice, &queueCount, nullptr );
		MW_ASSERT( queueCount >= 1, "No queue families found" );

		std::vector<VkQueueFamilyProperties> queueProps( queueCount );
		vkGetPhysicalDeviceQueueFamilyProperties( *info->pPhysDevice, &queueCount, queueProps.data() );

		std::vector<VkBool32> supportsPresent( queueCount );
		for ( uint32_t i = 0; i < queueCount; i++ )
		{
			vkGetPhysicalDeviceSurfaceSupportKHR( *info->pPhysDevice, i, m_Surface, &supportsPresent[i] );
		}

		u32 graphicsQueueNodeIndex = UINT32_MAX;
		u32 presentQueueNodeIndex = UINT32_MAX;

		for ( u32 i = 0; i < queueCount; i++ )
		{
			if ( ( queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) != 0 )
			{
				if ( graphicsQueueNodeIndex == UINT32_MAX )
				{
					graphicsQueueNodeIndex = i;
				}

				if ( supportsPresent[i] == VK_TRUE )
				{
					graphicsQueueNodeIndex = i;
					presentQueueNodeIndex = i;
					break;
				}
			}
		}

		if ( presentQueueNodeIndex == UINT32_MAX )
		{
			for ( u32 i = 0; i < queueCount; ++i )
			{
				if ( supportsPresent[i] == VK_TRUE )
				{
					presentQueueNodeIndex = i;
					break;
				}
			}
		}

		MW_ASSERT( graphicsQueueNodeIndex != UINT32_MAX, "No graphics queue family found" );
		MW_ASSERT( presentQueueNodeIndex != UINT32_MAX, "No present queue family found" );

		m_QueueNodeIndex = graphicsQueueNodeIndex;

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR( *info->pPhysDevice, m_Surface, &formatCount, nullptr );
		MW_ASSERT( formatCount > 0, "No surface formats found" );

		std::vector<VkSurfaceFormatKHR> surfaceFormats( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( *info->pPhysDevice, m_Surface, &formatCount, surfaceFormats.data() );

		if ( ( formatCount == 1 ) && ( surfaceFormats[0].format == VK_FORMAT_UNDEFINED ) )
		{
			m_ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			m_ColorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			bool found_B8G8R8A8_UNORM = false;
			for ( auto&& surfaceFormat : surfaceFormats )
			{
				if ( surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM )
				{
					m_ColorFormat = surfaceFormat.format;
					m_ColorSpace = surfaceFormat.colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
			}

			if ( !found_B8G8R8A8_UNORM )
			{
				m_ColorFormat = surfaceFormats[0].format;
				m_ColorSpace = surfaceFormats[0].colorSpace;
			}
		}		
	}

	void CVulkanSDLPresenter::CreateSurface( const IPresentationSurfaceCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium" );

		auto info = ( SVulkanSDLPresentationSurfaceCreationInfo* )pInfo;

		MW_INFO( "Create Surface" );

		if ( !SDL_Vulkan_CreateSurface( m_Window, *info->pInstance, nullptr, &m_Surface ) )
		{
			MW_ERROR( "Failed to create window surface: {}", SDL_GetError() );
		}
	}

	void CVulkanSDLPresenter::Init2( const IPresentationInitialization2Info* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium" );

		auto info = ( SVulkanSDLPresentationInitialization2Info* )pInfo;

		SwapChainSupportDetails swapChainSupport = info->pVulkanDevice->QuerySwapChainSupport(info->pPhysDevice, m_Surface);

		VkSurfaceFormatKHR surfaceFormat{};
		
		for ( const auto& availableFormat : swapChainSupport.Formats )
		{
			if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
			{
				surfaceFormat = availableFormat;
				break;
			}
			surfaceFormat = swapChainSupport.Formats[0];
		};
		
		m_ColorFormat = surfaceFormat.format;

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

		if ( !m_VSync )
		{
			for ( const auto& availablePresentMode : swapChainSupport.PresentModes )
			{
				if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
				{
					presentMode = availablePresentMode;
					break;
				}
			}
		}

		if ( presentMode == VK_PRESENT_MODE_MAILBOX_KHR )
		{
			MW_INFO( "Present mode: Mailbox" );
		}
		else
		{
			MW_INFO( "Present mode: V-Sync (FIFO)" );
		}
		
		VkExtent2D extent;
		
		if ( swapChainSupport.Capabilities.currentExtent.width != UINT32_MAX )
		{
			extent = swapChainSupport.Capabilities.currentExtent;
		}
		else
		{
			const auto& capabilities = swapChainSupport.Capabilities;

			extent = {
				.width = std::clamp(
					m_SwapchainExtent.Width,
					capabilities.minImageExtent.width,
					capabilities.maxImageExtent.width
				),
				.height = std::clamp(
					m_SwapchainExtent.Height,
					capabilities.minImageExtent.height,
					capabilities.maxImageExtent.height
				)
			};
		}

		u32 imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if ( swapChainSupport.Capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.Capabilities.maxImageCount )
		{
			imageCount = swapChainSupport.Capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfoVk = {};
		createInfoVk.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfoVk.surface = m_Surface;

		createInfoVk.minImageCount = imageCount;
		createInfoVk.imageFormat = surfaceFormat.format;
		createInfoVk.imageColorSpace = surfaceFormat.colorSpace;
		createInfoVk.imageExtent = extent;
		createInfoVk.imageArrayLayers = 1;
		createInfoVk.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = info->pVulkanDevice->FindPhysicalQueueFamilies();
		u32 queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily, indices.ComputeFamily, indices.TransferFamily };

		if ( indices.GraphicsFamily != indices.PresentFamily )
		{
			createInfoVk.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfoVk.queueFamilyIndexCount = 2;
			createInfoVk.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfoVk.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfoVk.queueFamilyIndexCount = 0;
			createInfoVk.pQueueFamilyIndices = nullptr;
		}

		createInfoVk.preTransform = swapChainSupport.Capabilities.currentTransform;
		createInfoVk.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfoVk.presentMode = presentMode;
		createInfoVk.clipped = VK_TRUE;

		createInfoVk.oldSwapchain = VK_NULL_HANDLE;

		MW_VK_CHECK( vkCreateSwapchainKHR( *info->pVulkanDevice->GetDevice(), &createInfoVk, nullptr, &m_Swapchain ), "Failed to create Swapchain" );

		vkGetSwapchainImagesKHR( *info->pVulkanDevice->GetDevice(), m_Swapchain, &imageCount, nullptr );
		m_SwapchainImages.resize( imageCount );

		for ( size_t i = 0; i < imageCount; i++ )
		{
			STextureCreateInfo textureInfo{};
			textureInfo.Format = ( EImageFormat )surfaceFormat.format;
			textureInfo.Extent = { extent.width, extent.height, 1 };
			textureInfo.GenerateImage = false;
			textureInfo.GenerateSampler = false;
			textureInfo.GenerateImageView = false;
			textureInfo.ImageLayout = MW_IMAGE_LAYOUT_UNDEFINED;
			textureInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			textureInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;

			m_SwapchainImages[i] = ITexture2D::Create( &textureInfo );
		};


		std::vector<VkImage> images( imageCount );
		vkGetSwapchainImagesKHR( *info->pVulkanDevice->GetDevice(), m_Swapchain, &imageCount, images.data() );

		for ( u32 i{}; i < imageCount; i++ )
		{
			STextureCreateInfo textureInfo {};
			textureInfo.Format = ( EImageFormat )surfaceFormat.format;
			textureInfo.Extent = { extent.width, extent.height, 1 };
			textureInfo.GenerateImage = false;
			textureInfo.GenerateSampler = false;
			textureInfo.GenerateImageView = false;
			textureInfo.ManageImage = false;
			textureInfo.ImageLayout = MW_IMAGE_LAYOUT_UNDEFINED;
			textureInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			textureInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;

			m_SwapchainImages[i] = ITexture2D::Create( &textureInfo );

			// mega freaky unsafe casting action
			const auto texture = m_SwapchainImages[i];
			auto vulkanTexture = texture.As<CVulkanTexture2D>();
			vulkanTexture->SetImage( &images[i] );
			
			VkImageViewCreateInfo viewInfo {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = *vulkanTexture->GetImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_ColorFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			// TODO: allocation callbacks
			MW_VK_CHECK( vkCreateImageView( *info->pDevice, &viewInfo, nullptr, vulkanTexture->GetImageView() ), "Failed to create Image View" );

			
		}

	}

	void CVulkanSDLPresenter::TransitionPresent( const IPresentationTransitionPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium" );

		auto info = ( SVulkanSDLPresentationTransitionPresentInfo* )pInfo;
		auto texture = m_SwapchainImages[info->ImageIndex].As<CVulkanTexture2D>();
	
		if ( texture->Layout == MW_IMAGE_LAYOUT_PRESENT_SRC_KHR )
			return;

		TransitionImageLayout3(
			*info->pCmdBuffer,
			*texture->GetImage(),
			( VkImageLayout )texture->Layout,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		texture->Layout = MW_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	void CVulkanSDLPresenter::TransitionRender( const IPresentationTransitionRenderInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium" );

		auto info = ( SVulkanSDLPresentationTransitionPresentInfo* )pInfo;

		auto texture = m_SwapchainImages[info->ImageIndex].As<CVulkanTexture2D>();

		if ( texture->Layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
			return;

		TransitionImageLayout3(
			*info->pCmdBuffer,
			*texture->GetImage(),
			( VkImageLayout )texture->Layout,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		);

		texture->Layout = MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	void CVulkanSDLPresenter::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Shutdown CVulkanSDLPresenter" );

		auto device = CVulkanContext::GetDevice();
		vkDeviceWaitIdle( *device->GetDevice() );

		m_SwapchainImages.clear();

		// TODO: Allocation Callbacks
		if ( m_Swapchain )
		{
			vkDestroySwapchainKHR( *device->GetDevice(), m_Swapchain, nullptr );
			m_Swapchain = nullptr;
		}

		if ( m_Surface )
			vkDestroySurfaceKHR( *CVulkanContext::GetInstance(), m_Surface, nullptr);


	}

	bool CVulkanSDLPresenter::OnResize( SEvent& event )
	{
		MW_PROFILE_FUNC;
		return false;
	}

	NODISCARD u32 CVulkanSDLPresenter::Acquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium" );

		auto info = ( SVulkanSDLPresentationAcquisitionInfo* )pInfo;

		vkWaitForFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence,
			VK_TRUE,
			std::numeric_limits<uint64_t>::max() );

		vkAcquireNextImageKHR(
			*info->pVulkanDevice->GetDevice(),
			m_Swapchain,
			std::numeric_limits<uint64_t>::max(),
			*info->pImageAvailableSemaphore,
			VK_NULL_HANDLE,
			&m_CurrentImageIndex );

		return m_CurrentImageIndex;

	}
	
	void CVulkanSDLPresenter::Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_SDL, "Invalid Presentation Medium" );

		auto info = ( SVulkanSDLPresentationPresentInfo* )pInfo;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = { info->pRenderFinishedSemaphore };

		VkSwapchainKHR swapChains[] = { m_Swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = info->pImageIndex;

		auto result = vkQueuePresentKHR( *info->pPresentQueue, &presentInfo );

		// Mark this image as having been presented at least once
		m_ImagePresentedOnce[*info->pImageIndex] = true;

		vkQueueWaitIdle( *info->pPresentQueue );
	}

}

