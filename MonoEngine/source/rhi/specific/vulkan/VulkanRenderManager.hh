#pragma once
#include <common/Base.hh>

#include <renderer/StaticRenderer.hh>
#include <core/Application.hh>

#include <volk/volk.h>



namespace Monoworks::RHI
{
	struct SVulkanFrameData 
	{
		VkCommandPool	GraphicsCommandPool;
		VkCommandBuffer GraphicsCommandBuffer;

		VkSemaphore		QtReadFinishedSemaphore;
		VkSemaphore		ImageAvailableSemaphore;
		VkSemaphore		GraphicsSubmitSemaphore; // NOTE: Signaled when the GPass/the last graphics pass finishes and gets submitted
		VkSemaphore		RenderFinishedSemaphore; // NOTE: Signaled when all Passes (Pre-, Main-, Resolution- & Post-Passes) have been submitted

		VkFence			InFlightFence;
	};

	struct SVulkanWorkerData 
	{
		VkCommandPool	GraphicsCommandPools[MFIF];
		VkCommandBuffer GraphicsCommandBuffers[MFIF];
	};

	class CVulkanRenderManager
	{
	public:
		static void Init() NOEXCEPT;
		static void Shutdown() NOEXCEPT;

		// Graphics
		static void BeginRootGraphicsCommandBuffer( u32 frameIndex )	NOEXCEPT;
		static void EndRootGraphicsCommandBuffer( u32 frameIndex )		NOEXCEPT;
		static void SubmitRootGraphicsCommandBuffer( u32 frameIndex )	NOEXCEPT;

		static void BeginWorkerGraphicsCommandBuffers( u32 frameIndex ) NOEXCEPT;
		static void EndWorkerGraphicsCommandBuffers( u32 frameIndex )	NOEXCEPT;


		NODISCARD static VkCommandBuffer* GetRootGraphicsCommandBuffer( u32 frameIndex )				NOEXCEPT { return &m_RootFrameData[frameIndex].GraphicsCommandBuffer; };
		NODISCARD static VkCommandBuffer* GetWorkerCommandBuffer( u32 workerThreadID, u32 frameIndex )	NOEXCEPT { return &m_WorkerRenderData[workerThreadID].GraphicsCommandBuffers[frameIndex]; };

		NODISCARD static VkSemaphore* GetImageAvailableSemaphore( u32 frameIndex )						NOEXCEPT { return &m_RootFrameData[frameIndex].ImageAvailableSemaphore; };
		NODISCARD static VkSemaphore* GetRenderFinishedSemaphore( u32 frameIndex )						NOEXCEPT { return &m_RootFrameData[frameIndex].RenderFinishedSemaphore; };
		NODISCARD static VkSemaphore* GetGraphicsSubmitSemaphore( u32 frameIndex )						NOEXCEPT { return &m_RootFrameData[frameIndex].GraphicsSubmitSemaphore; };

		NODISCARD static VkSemaphore* GetQtReadFinishedSemaphore( u32 frameIndex )						NOEXCEPT { if ( !CApplication::GetCreateInfos()->UseQt ) { MW_API_ERROR( "Illegal function call: Accessing Qt specific render elements without UseQt flag specified. " ); return nullptr; } return &m_RootFrameData[frameIndex].QtReadFinishedSemaphore; }

		NODISCARD static VkFence* GetInFlightFence( u32 frameIndex )									NOEXCEPT { return &m_RootFrameData[frameIndex].InFlightFence; };

		NODISCARD static VkCommandBuffer* GetCurrentRootGraphicsCommandBuffer()							NOEXCEPT { return &m_RootFrameData[Monoworks::CStaticRenderer::GetCurrentFrameIndex()].GraphicsCommandBuffer; };
		NODISCARD static VkCommandBuffer* GetCurrentWorkerGraphicsCommandBuffer( u32 workerThreadID )	NOEXCEPT { return &m_WorkerRenderData[workerThreadID].GraphicsCommandBuffers[Monoworks::CStaticRenderer::GetCurrentFrameIndex()]; }

		NODISCARD static const std::vector<SVulkanWorkerData>& GetWorkerFrameData()						NOEXCEPT { return m_WorkerRenderData; }; 

	private:

		
		static SVulkanFrameData m_RootFrameData[MFIF];
		static std::vector<SVulkanWorkerData> m_WorkerRenderData;

	};
}
