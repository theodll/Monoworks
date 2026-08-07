#ifdef MW_PLATFORM_WINDOWS
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32 1 
#include <volk/volk.h>
#define VMA_EXTERNAL_MEMORY_WIN32 1
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


		for ( u32 i {}; i < m_PresentationImages.size(); i++ )
		{
			auto texture = m_PresentationImages[i].As<CVulkanTexture2D>();
			auto allocator = CVulkanContext::GetAllocator();
#ifdef MW_PLATFORM_WINDOWS
			vmaGetMemoryWin32Handle2(
				*allocator,
				*texture->GetVmaAllocation(),
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
				nullptr,
				&m_PresentationImageWin32Handles[i] );
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

		if ( info->RenderFinishedSemaphoreCount < MFIF )
			MW_ASSERT( false, "Insufficient Number of RenderFinishedSemaphores" );

		for (u32 i{}; i < info->RenderFinishedSemaphoreCount; i++ )
		{
#ifdef MW_PLATFORM_WINDOWS
			VkSemaphoreGetWin32HandleInfoKHR getSemaphoreHandleInfo{};
			getSemaphoreHandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
			getSemaphoreHandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
			getSemaphoreHandleInfo.semaphore = *info->pRenderFinishedSemaphores[i];

			vkGetSemaphoreWin32HandleKHR( *info->pVulkanDevice->GetDevice(), &getSemaphoreHandleInfo, &m_RenderFinishedSemaphoreWin32Handles[i] );

#else
			VkSemaphoreGetFdInfoKHR getSemaphoreFdInfo {};
			getSemaphoreFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
			getSemaphoreFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
			getSemaphoreFdInfo.semaphore = *info->pRenderFinishedSemaphores[i];

			vkGetSemaphoreFdKHR( *info->pVulkanDevice->GetDevice(), &getSemaphoreFdInfo, &m_RenderFinishedSemaphoreFds[i] );
#endif
		}



	};

	void CVulkanQtPresenter::Shutdown() NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		m_PresentationImages.clear();

		for (u32 i{}; i < m_PresentationImages.size(); i++ )
		{
#ifdef MW_PLATFORM_WINDOWS
			CloseHandle( m_PresentationImageWin32Handles[i] );
#else
			m_PresentationImageFd[i] = -1;
#endif
		}

	};

	bool CVulkanQtPresenter::OnResize( SEvent& event ) 
	{
		MW_PROFILE_FUNC;
		return false;
	};

	NODISCARD u32 CVulkanQtPresenter::Acquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationAcquisitionInfo* )pInfo;

		if ( m_CurrentImageIndex >= m_PresentationImages.size() )
			m_CurrentImageIndex = 0;

		if ( m_PresentationImages.size() < MFIF )
			MW_ASSERT("Insufficient Presentation Images.");
		// TODO: something with this
		/*MW_VK_CHECK(vkWaitForFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence,
			VK_TRUE,
			UINT64_MAX ), "Failed to wait for InFlightFence" );

		vkResetFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence );
			*/
		m_CurrentImageIndex = ( m_CurrentImageIndex + 1 ) % MFIF;
		 
		// TODO: change to timeline semaphores
		// empty submit to ONLY signal the semaphore
		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 0;
		submitInfo.pCommandBuffers = nullptr;
		submitInfo.pSignalSemaphores = info->pImageAvailableSemaphore;
		submitInfo.signalSemaphoreCount = 1;

		vkQueueSubmit(
			*info->pGraphicsQueue,
			1,
			&submitInfo,
			nullptr );

		return m_CurrentImageIndex;
	};

	void CVulkanQtPresenter::TransitionRender( const IPresentationTransitionRenderInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationTransitionRenderInfo* )pInfo;

		auto texture = m_PresentationImages[info->ImageIndex].As<CVulkanTexture2D>();

		if ( texture->Layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
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

		if ( texture->Layout == MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
			return;

		TransitionImageLayout2(
			*info->pCmdBuffer,
			*texture->GetImage(),
			( VkImageLayout )texture->Layout,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
		);

		texture->Layout = MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	};

	void CVulkanQtPresenter::Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationPresentInfo* )pInfo;
	};

}