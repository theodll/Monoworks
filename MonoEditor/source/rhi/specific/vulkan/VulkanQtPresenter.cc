#ifdef MW_PLATFORM_WINDOWS
#include <Windows.h>
#undef ERROR

#ifndef VK_USE_PLATFORM_WIN32
#define VK_USE_PLATFORM_WIN32 1 
#endif

#include <volk/volk.h>
#ifndef VMA_EXTERNAL_MEMORY_WIN32
#define VMA_EXTERNAL_MEMORY_WIN32 1
#endif

#endif

#include "VulkanQtPresenter.hh"

#include <Monoworks.hh>

#include <rhi/agnostic/Texture.hh>

#include <rhi/specific/vulkan/VulkanPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>



namespace Monoworks::RHI 
{
	void CVulkanQtPresenter::Init2( const IPresentationInitialization2Info* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationInitialization2Info* )pInfo;

		// TODO: check this somehow
		m_ColorImageFormat = MW_FORMAT_B8G8R8A8_SRGB;

		m_PresentationImages.resize( MFIF );
		for ( auto& texture : m_PresentationImages )
		{
			STextureCreateInfo createInfo{};
			createInfo.Format = m_ColorImageFormat; 
			createInfo.Flags = MW_TEXTURE_CREATION_FLAG_ENABLE_MEMORY_EXPORTING;
			createInfo.ImageLayout = MW_IMAGE_LAYOUT_UNDEFINED;
			createInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | MW_IMAGE_USAGE_SAMPLED_BIT;
			createInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			createInfo.Extent = { m_SwapchainExtent.Width, m_SwapchainExtent.Height, 1 };

			texture = ITexture2D::Create( &createInfo );
		}


		for ( u32 i{}; i < m_PresentationImages.size(); i++ )
		{
			auto texture = m_PresentationImages[i].As<CVulkanTexture2D>();
			auto allocator = CVulkanContext::GetAllocator();

#ifdef MW_PLATFORM_WINDOWS

			
			VkResult res = vmaGetMemoryWin32Handle2(
				*allocator,
				*texture->GetVmaAllocation(),
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
				nullptr,
				&m_PresentationImageWin32Handles[i] );

			if ( !( res == VK_SUCCESS && m_PresentationImageWin32Handles[i] != nullptr ) )
			{
				MW_ERROR( "Failed to export Win32 Handle of Presentation Texture at index {}", i );
				MW_DEBUG_BREAK;
			}
#else

			VmaAllocationInfo2 allocInfo {};
			vmaGetAllocationInfo2( *allocator, *texture->GetVmaAllocation(), &allocInfo );

			VkMemoryGetFdInfoKHR getFdInfo{};
			getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
			getFdInfo.memory = allocInfo.allocationInfo.deviceMemory;
			getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

			vkGetMemoryFdKHR( *info->pVulkanDevice->GetDevice(), &getFdInfo, &m_PresentationImageFds[i] );
#endif
		}

		MW_ASSERT( info->RenderFinishedSemaphoreCount >= MFIF, "Insufficient Number of RenderFinishedSemaphores" );
		MW_ASSERT( info->QtReadFinishedSemaphoreCount >= MFIF, "Insufficient Number of QtReadFinishedSemaphores" );


		for (u32 i{}; i < MFIF; i++ )
		{
#ifdef MW_PLATFORM_WINDOWS
			VkSemaphoreGetWin32HandleInfoKHR getRenderFinishedSemaphoreHandleInfo{};
			getRenderFinishedSemaphoreHandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
			getRenderFinishedSemaphoreHandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
			getRenderFinishedSemaphoreHandleInfo.semaphore = *info->pRenderFinishedSemaphores[i];

			vkGetSemaphoreWin32HandleKHR( *info->pVulkanDevice->GetDevice(), &getRenderFinishedSemaphoreHandleInfo, &m_RenderFinishedSemaphoreWin32Handles[i] );

#else
			VkSemaphoreGetFdInfoKHR getRenderFinishedSemaphoreFdInfo{};
			getRenderFinishedSemaphoreFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
			getRenderFinishedSemaphoreFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
			getRenderFinishedSemaphoreFdInfo.semaphore = *info->pRenderFinishedSemaphores[i];

			MW_VK_CHECK( vkGetSemaphoreFdKHR( *info->pVulkanDevice->GetDevice(), &getRenderFinishedSemaphoreFdInfo, &m_RenderFinishedSemaphoreFds[i] ), "Failed to export Qt Read Finished Semaphore at index {}", i );
#endif
		}

		for ( u32 i{}; i < info->QtReadFinishedSemaphoreCount; i++ )
		{
#ifdef MW_PLATFORM_WINDOWS
			VkSemaphoreGetWin32HandleInfoKHR getQtReadFinishedSemaphoreHandleInfo{};
			getQtReadFinishedSemaphoreHandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
			getQtReadFinishedSemaphoreHandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
			getQtReadFinishedSemaphoreHandleInfo.semaphore = *info->pQtReadFinishedSemaphores[i];

			vkGetSemaphoreWin32HandleKHR( *info->pVulkanDevice->GetDevice(), &getQtReadFinishedSemaphoreHandleInfo, &m_QtReadFinishedSemaphoreWin32Handles[i] );
#else
			VkSemaphoreGetFdInfoKHR getQtReadFinishedSemaphoreFdInfo{};
			getQtReadFinishedSemaphoreFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
			getQtReadFinishedSemaphoreFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
			getQtReadFinishedSemaphoreFdInfo.semaphore = *info->pQtReadFinishedSemaphores[i];

			MW_VK_CHECK( vkGetSemaphoreFdKHR( *info->pVulkanDevice->GetDevice(), &getQtReadFinishedSemaphoreFdInfo, &m_QtReadFinishedSemaphoreFds[i] ), "Failed to export Qt Read Finished Semaphore at index {}", i );
#endif
		}


	};

	void CVulkanQtPresenter::Shutdown() NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		for (u32 i{}; i < m_PresentationImages.size(); i++ )
		{
#ifdef MW_PLATFORM_WINDOWS
			CloseHandle( m_PresentationImageWin32Handles[i] );
			CloseHandle( m_RenderFinishedSemaphoreWin32Handles[i] );
			CloseHandle( m_QtReadFinishedSemaphoreWin32Handles[i] );
			m_PresentationImageWin32Handles[i] = nullptr;
			m_RenderFinishedSemaphoreWin32Handles[i] = nullptr;
			m_QtReadFinishedSemaphoreWin32Handles[i] = nullptr;
#else
			close( m_PresentationImageFds[i] );
			close( m_RenderFinishedSemaphoreFds[i] );
			close( m_QtReadFinishedSemaphoreFds[i] );
			m_PresentationImageFds[i] = -1;
			m_RenderFinishedSemaphoreFds[i] = -1;
			m_QtReadFinishedSemaphoreFds[i] = -1;
#endif
		}
		m_PresentationImages.clear();

	};

	bool CVulkanQtPresenter::OnResize( MAYBE_UNUSED SEvent& event ) 
	{
		MW_PROFILE_FUNC;
		return false;
	};

	NODISCARD u32 CVulkanQtPresenter::Acquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationAcquisitionInfo* )pInfo;

		if ( m_PresentationImages.size() < MFIF )
			MW_ASSERT( false, "Insufficient Presentation Images." );

		m_CurrentImageIndex = ( m_CurrentImageIndex + 1 ) % MFIF;

		MW_VK_CHECK( vkWaitForFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence, 
			VK_TRUE,
			UINT64_MAX ), "Failed to wait for InFlightFence" );

		MW_VK_CHECK( vkResetFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence ), "Failed to reset InFlightFence" );

		// TODO: use timeline semaphores
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 0;
		submitInfo.pCommandBuffers = nullptr;
		submitInfo.pSignalSemaphores = info->pImageAvailableSemaphore;
		submitInfo.signalSemaphoreCount = ( info->pImageAvailableSemaphore != nullptr ) ? 1 : 0;

		MW_VK_CHECK( vkQueueSubmit(
			*info->pGraphicsQueue,
			1,
			&submitInfo,
			*info->pInFlightFence 
		), "Failed to submit acquire semaphore trigger" );

		return m_CurrentImageIndex;
	};

	void CVulkanQtPresenter::TransitionRender( const IPresentationTransitionRenderInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationTransitionRenderInfo* )pInfo;

		auto texture = m_PresentationImages[info->ImageIndex].As<CVulkanTexture2D>();

		if ( texture->Layout == MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
			return;

		TransitionImageLayout2(
			*info->pCmdBuffer,
			*texture->GetImage(),
			( VkImageLayout )texture->Layout,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		);

		texture->Layout = MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
	};

	void CVulkanQtPresenter::TransitionPresent( const IPresentationTransitionPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationTransitionPresentInfo* )pInfo;

		auto texture = m_PresentationImages[info->ImageIndex].As<CVulkanTexture2D>();

		if ( texture->Layout == MW_IMAGE_LAYOUT_GENERAL )
			return;

		TransitionImageLayout2(
			*info->pCmdBuffer,
			*texture->GetImage(),
			( VkImageLayout )texture->Layout,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		texture->Layout = MW_IMAGE_LAYOUT_GENERAL;
	};

	void CVulkanQtPresenter::Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );
	};

}