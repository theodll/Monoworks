#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/Presenter.hh>

namespace Monoworks::RHI 
{
	// Presenter Implementation are in the Application Specific RHI Component

	struct SVulkanSDLPresentationInitializationInfo : public IPresentationInitializationInfo 
	{
		SVulkanSDLPresentationInitializationInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; };
		CVulkanDevice*			pVulkanDevice;
		const VkInstance*		pInstance;
		const VkPhysicalDevice* pPhysDevice;
		const VkDevice*			pDevice;

	};

	struct SVulkanSDLPresentationInitialization2Info : public IPresentationInitialization2Info
	{
		SVulkanSDLPresentationInitialization2Info() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; };
		CVulkanDevice*			pVulkanDevice;
		VkPhysicalDevice		pPhysDevice;
		const VkDevice*			pDevice;
	};

	struct SVulkanSDLPresentationAcquisitionInfo : public IPresentationAcquisitionInfo 
	{
		SVulkanSDLPresentationAcquisitionInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
		CVulkanDevice*			pVulkanDevice;
		const VkPhysicalDevice* pPhysDevice;
		const VkDevice*			pDevice;
		VkFence*				pInFlightFence;
		VkSemaphore*			pImageAvailableSemaphore;
	};

	struct SVulkanSDLPresentationPresentInfo : public IPresentationPresentInfo 
	{
		SVulkanSDLPresentationPresentInfo() { *const_cast<EPresentationMedium*>( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
		CVulkanDevice*			pVulkanDevice;
		const VkPhysicalDevice* pPhysDevice;
		const VkDevice*			pDevice;
		const VkQueue*			pPresentQueue;
		VkSemaphore*			pRenderFinishedSemaphore;
		u32*					pImageIndex;
	};

	struct SVulkanSDLPresentationSurfaceCreationInfo : public IPresentationSurfaceCreationInfo
	{
		SVulkanSDLPresentationSurfaceCreationInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; } 
		const VkInstance*		pInstance;
	};


	struct SVulkanSDLPresentationTransitionPresentInfo : public IPresentationTransitionPresentInfo
	{
		SVulkanSDLPresentationTransitionPresentInfo() { *const_cast< EPresentationMedium* >( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
		const VkCommandBuffer*	pCmdBuffer;
		u32						ImageIndex;
	};

	struct SVulkanSDLPresentationTransitionRenderInfo : public IPresentationTransitionRenderInfo
	{
		SVulkanSDLPresentationTransitionRenderInfo() { *const_cast< EPresentationMedium* >( &Medium ) = MW_PRESENTATION_MEDIUM_VULKAN_SDL; }
		const VkCommandBuffer*	pCmdBuffer;
		u32						ImageIndex;
	};

	// Qt

	struct SVulkanQtPresentationInitializationInfo : public IPresentationInitializationInfo
	{
		SVulkanQtPresentationInitializationInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; };
	};

	struct SVulkanQtPresentationInitialization2Info : public IPresentationInitialization2Info
	{
		SVulkanQtPresentationInitialization2Info() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; };
		CVulkanDevice*			pVulkanDevice;
		VkSemaphore**			pImageAvailableSemaphores; // for handle exporting 
		VkSemaphore**			pRenderFinishedSemaphores; // also for handle exporting
		u32						ImageAvailableSemaphoreCount;
		u32						RenderFinishedSemaphoreCount;
	};

	struct SVulkanQtPresentationAcquisitionInfo : public IPresentationAcquisitionInfo
	{
		SVulkanQtPresentationAcquisitionInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; }
		CVulkanDevice*			pVulkanDevice; 
		VkFence*				pInFlightFence;
		VkSemaphore*			pImageAvailableSemaphore;
		VkQueue*				pGraphicsQueue; // for semaphore signalisation
	};

	struct SVulkanQtPresentationPresentInfo : public IPresentationPresentInfo
	{
		SVulkanQtPresentationPresentInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; }
		u32*					pImageIndex;
	};

	struct SVulkanQtPresentationSurfaceCreationInfo : public IPresentationSurfaceCreationInfo
	{
		SVulkanQtPresentationSurfaceCreationInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; }
	};


	struct SVulkanQtPresentationTransitionPresentInfo : public IPresentationTransitionPresentInfo
	{
		SVulkanQtPresentationTransitionPresentInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; }
		const VkCommandBuffer*	pCmdBuffer;
		u32						ImageIndex;
	};

	struct SVulkanQtPresentationTransitionRenderInfo : public IPresentationTransitionRenderInfo
	{
		SVulkanQtPresentationTransitionRenderInfo() { *const_cast< EPresentationMedium* >(&Medium) = MW_PRESENTATION_MEDIUM_VULKAN_QT; }
		const VkCommandBuffer*	pCmdBuffer;
		u32						ImageIndex;
	};
}
