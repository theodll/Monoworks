#pragma once
#include <common/Base.hh>

#include <renderer/StaticRenderer.hh>

#include <volk/volk.h>



namespace Monoworks::RHI
{
	struct SVulkanFrameData 
	{
		VkCommandBuffer CommandBuffer;
		VkCommandPool	CommandPool;

		VkSemaphore		ImageAvailableSemaphore;
		VkSemaphore		RenderFinishedSemaphore;

		VkFence			InFlightFence;
	};

	struct SVulkanWorkerData 
	{
		VkCommandPool	CommandPools[MFIF];
		VkCommandBuffer CommandBuffers[MFIF];		
	};

	class CVulkanRenderManager
	{
	public:
		static void Init() NOEXCEPT;
		static void Shutdown() NOEXCEPT;

		static void BeginRootCommandBuffer( u32 frameIndex )	NOEXCEPT;
		static void EndRootCommandBuffer( u32 frameIndex )		NOEXCEPT;
		static void SubmitRootCommandBuffer( u32 frameIndex )	NOEXCEPT;

		static void BeginWorkerCommandBuffers( u32 frameIndex ) NOEXCEPT;
		static void EndWorkerCommandBuffers( u32 frameIndex )	NOEXCEPT;

		NODISCARD static VkCommandBuffer* GetRootCommandBuffer( u32 frameIndex )							NOEXCEPT { return &m_RootFrameData[frameIndex].CommandBuffer; };
		NODISCARD static VkCommandBuffer* GetWorkerCommandBuffer( u32 workerThreadID, u32 frameIndex )	NOEXCEPT { return &m_WorkerRenderData[workerThreadID].CommandBuffers[frameIndex]; };

		NODISCARD static VkSemaphore* GetImageAvailableSemaphore( u32 frameIndex )					NOEXCEPT { return &m_RootFrameData[frameIndex].ImageAvailableSemaphore; };
		NODISCARD static VkSemaphore* GetRenderFinishedSemaphore( u32 frameIndex )					NOEXCEPT { return &m_RootFrameData[frameIndex].RenderFinishedSemaphore; };

		NODISCARD static VkFence* GetInFlightFence( u32 frameIndex )								NOEXCEPT { return &m_RootFrameData[frameIndex].InFlightFence; };

		NODISCARD static VkCommandBuffer* GetCurrentRootCommandBuffer()									NOEXCEPT { return &m_RootFrameData[Monoworks::CStaticRenderer::GetCurrentFrameIndex()].CommandBuffer; };
		NODISCARD static VkCommandBuffer* GetCurrentWorkerCommandBuffer( u32 workerThreadID )				NOEXCEPT { return &m_WorkerRenderData[workerThreadID].CommandBuffers[Monoworks::CStaticRenderer::GetCurrentFrameIndex()]; }

	private:

		
		static SVulkanFrameData m_RootFrameData[MFIF];
		static std::vector<SVulkanWorkerData> m_WorkerRenderData;

	};
}
