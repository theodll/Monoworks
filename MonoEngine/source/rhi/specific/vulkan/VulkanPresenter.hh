#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/Presenter.hh>

namespace Monoworks::RHI 
{
	struct SVulkanSDLPresentationInitializationInfo : public IPresentationInitializationInfo 
	{
		SVulkanSDLPresentationInitializationInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; };
		VkInstance* Instance;
		CVulkanDevice* VulkanDevice;

	};

	struct SVulkanSDLPresentationInitialization2Info : public IPresentationInitialization2Info
	{
		SVulkanSDLPresentationInitialization2Info() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; };
		CVulkanDevice* VulkanDevice;
	};

	struct SVulkanSDLPresentationAcquisitionInfo : public IPresentationAcquisitionInfo 
	{
		SVulkanSDLPresentationAcquisitionInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
	};

	struct SVulkanSDLPresentationPresentInfo : public IPresentationPresentInfo 
	{
		SVulkanSDLPresentationPresentInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
	};
}
