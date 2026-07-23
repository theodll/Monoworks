#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/Presenter.hh>

namespace Monoworks::RHI 
{
	struct SVulkanSDLPresentationInitializationInfo : public IPresentationInitializationInfo 
	{
		SVulkanSDLPresentationInitializationInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; };
		VkInstance* pInstance;
		CVulkanDevice* pVulkanDevice;

	};

	struct SVulkanSDLPresentationInitialization2Info : public IPresentationInitialization2Info
	{
		SVulkanSDLPresentationInitialization2Info() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; };
		CVulkanDevice* pVulkanDevice;
	};

	struct SVulkanSDLPresentationAcquisitionInfo : public IPresentationAcquisitionInfo 
	{
		SVulkanSDLPresentationAcquisitionInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
		CVulkanDevice* pVulkanDevice;
		VkFence* pInFlightFence;
		VkSemaphore* pImageAvailableSemaphore;
	};

	struct SVulkanSDLPresentationPresentInfo : public IPresentationPresentInfo 
	{
		SVulkanSDLPresentationPresentInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
		CVulkanDevice* pVulkanDevice;
		VkQueue* pPresentQueue;
		VkSemaphore* pRenderFinishedSemaphore;
		u32* pImageIndex;
	};
}
