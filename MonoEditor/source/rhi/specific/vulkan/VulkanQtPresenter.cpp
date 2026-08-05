#include <Monoworks.hh>

#include <rhi/agnostic/Texture.hh>

#include <rhi/specific/vulkan/VulkanPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>

#include "VulkanQtPresenter.hh"

#ifdef MW_PLATFORM_WINDOWS
#include <Windows.h>
#include <volk/volk.h>
#endif

namespace Monoworks::RHI 
{
	void CVulkanQtPresenter::Init2( const IPresentationInitialization2Info* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationInitialization2Info* )pInfo;

		// TODO: check this somehow
		m_ColorImageFormat = MW_FORMAT_B8G8R8A8_SRGB;

		for ( auto& texture : m_PresentationImages )
		{
			STextureCreateInfo createInfo{};
			createInfo.Format = m_ColorImageFormat; 
			createInfo.Flags = MW_TEXTURE_CREATION_FLAG_ENABLE_MEMORY_EXPORTING;
			createInfo.ImageLayout = MW_IMAGE_LAYOUT_UNDEFINED;
			createInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			createInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			createInfo.Extent = { m_SwapchainExtent.Width, m_SwapchainExtent.Height, 1 };

			texture = ITexture2D::Create( &createInfo );
		}

#ifdef MW_PLATFORM_WINDOWS
		for ( u32 i {}; i < m_PresentationImages.size(); i++ )
		{
			auto texture = m_PresentationImages[i].As<CVulkanTexture2D>();

			VmaAllocationInfo allocInfo;
			vmaGetAllocationInfo( *CVulkanContext::GetAllocator(), *texture->GetVmaAllocation(), &allocInfo );

			VkMemoryGetWin32HandleInfoKHR getHandleInfo{};
			getHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
			getHandleInfo.memory = allocInfo.deviceMemory;
			getHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

			vkGetMemoryWin32HandleKHR( );
		}
#endif
	};

	void CVulkanQtPresenter::Shutdown() NOEXCEPT 
	{
		MW_PROFILE_FUNC;
	};

	bool CVulkanQtPresenter::OnResize( SEvent& event ) 
	{
		MW_PROFILE_FUNC;
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

		MW_VK_CHECK( vkWaitForFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence,
			VK_TRUE,
			UINT64_MAX ), "Failed to wait for InFlightFence at index {}", m_CurrentImageIndex );

		vkResetFences(
			*info->pVulkanDevice->GetDevice(),
			1,
			info->pInFlightFence );

		m_CurrentImageIndex = ( m_CurrentImageIndex + 1 ) % MFIF;

		// empty submit to signal the semaphore
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
	};

	void CVulkanQtPresenter::TransitionPresent( const IPresentationTransitionPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationTransitionPresentInfo* )pInfo;
	};

	void CVulkanQtPresenter::Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		MW_ASSERT( pInfo->Medium == MW_PRESENTATION_MEDIUM_VULKAN_QT, "Invalid Presentation Medium" );

		auto info = ( SVulkanQtPresentationPresentInfo* )pInfo;
	};

}