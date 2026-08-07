#include <mwpch.hh>

#include <core/Application.hh>
#include <renderer/StaticRenderer.hh>

#include <rhi/specific/vulkan/VulkanContext.hh>
#include <rhi/specific/vulkan/VulkanPresenter.hh>

#include "VulkanRenderManager.hh"


namespace Monoworks::RHI 
{
	SVulkanFrameData			   CVulkanRenderManager::m_RootFrameData[MFIF];
	std::vector<SVulkanWorkerData> CVulkanRenderManager::m_WorkerRenderData;

	void CVulkanRenderManager::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initializa CVulkanRenderManager" );
		
		VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo{};
		exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
		if ( !CApplication::GetCreateInfos()->UseSwapchain )
		{

#ifdef MW_PLATFORM_WINDOWS
			exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
			exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
		}

		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if ( !CApplication::GetCreateInfos()->UseSwapchain )
			semaphoreCreateInfo.pNext = &exportSemaphoreCreateInfo;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		auto device = CVulkanContext::GetDevice();
		for ( auto& frameData : m_RootFrameData )
		{
			// TODO: allocation callbacks
			MW_VK_CHECK( vkCreateSemaphore( *device->GetDevice(), &semaphoreCreateInfo, nullptr, &frameData.ImageAvailableSemaphore), "Failed to create ImageAvailableSemaphore.");
			MW_VK_CHECK( vkCreateSemaphore( *device->GetDevice(), &semaphoreCreateInfo, nullptr, &frameData.RenderFinishedSemaphore), "Failed to create RenderFinishedSemaphore.");
			MW_VK_CHECK( vkCreateFence( *device->GetDevice(), &fenceCreateInfo, nullptr, &frameData.InFlightFence), "Failed to create InFlightFence.");

			VkCommandPoolCreateInfo poolCreateInfo{};
			poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolCreateInfo.queueFamilyIndex = device->GetGraphicsQueueFamilyIndex();
			poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			MW_VK_CHECK( vkCreateCommandPool( *device->GetDevice(), &poolCreateInfo, nullptr, &frameData.CommandPool ), "Failed to create CommandPool." );

			VkCommandBufferAllocateInfo allocCreateInfo{};
			allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocCreateInfo.commandPool = frameData.CommandPool;
			allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocCreateInfo.commandBufferCount = 1;

			MW_VK_CHECK( vkAllocateCommandBuffers( *device->GetDevice(), &allocCreateInfo, &frameData.CommandBuffer ), "Failed to allocate CommandBuffer." );
		}

		for ( auto& workerData : m_WorkerRenderData )
		{
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = device->GetGraphicsQueueFamilyIndex();
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			for ( auto i{0}; i < MFIF; i++ )
			{
				MW_VK_CHECK( vkCreateCommandPool( *device->GetDevice(), &poolInfo, nullptr, &workerData.CommandPools[i] ), "Failed to create CommandPool.");
			
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = workerData.CommandPools[i];
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				allocInfo.commandBufferCount = 1;

				MW_VK_CHECK( vkAllocateCommandBuffers( *device->GetDevice(), &allocInfo, &workerData.CommandBuffers[i] ), "Failed to allocate CommandBuffer.");
			}
		}


	};

	void CVulkanRenderManager::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = *CVulkanContext::GetDevice()->GetDevice();
		
		vkDeviceWaitIdle( device );

		for ( auto& workerData : m_WorkerRenderData )
		{
			for ( auto& commandPool : workerData.CommandPools )
			{
				if ( commandPool )
					vkDestroyCommandPool( device, commandPool, nullptr);
			}
		}

		m_WorkerRenderData.clear();

		for ( auto& frameData : m_RootFrameData )
		{
			if ( frameData.ImageAvailableSemaphore )
				vkDestroySemaphore( device, frameData.ImageAvailableSemaphore, nullptr );
			
			if ( frameData.RenderFinishedSemaphore )
				vkDestroySemaphore( device, frameData.RenderFinishedSemaphore, nullptr );

			if ( frameData.InFlightFence )
				vkDestroyFence( device, frameData.InFlightFence, nullptr );

			if ( frameData.CommandPool )
				vkDestroyCommandPool( device, frameData.CommandPool, nullptr );
		}
		MW_INFO( "Shutdown CVulkanRenderManager" );
	};

	void CVulkanRenderManager::BeginRootCommandBuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;


		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer( m_RootFrameData[frameIndex].CommandBuffer, &beginInfo );


		auto presenter = CVulkanContext::GetPresenter();
		if ( CApplication::GetCreateInfos()->UseSDL )
		{
			SVulkanSDLPresentationTransitionRenderInfo renderInfo{};
			renderInfo.pCmdBuffer = CVulkanRenderManager::GetRootCommandBuffer( frameIndex );
			renderInfo.ImageIndex = CStaticRenderer::GetCurrentImageIndex();

			presenter->TransitionRender( &renderInfo );
		} 
		else if ( CApplication::GetCreateInfos()->UseQt )
		{
			SVulkanQtPresentationTransitionRenderInfo renderInfo {};
			renderInfo.pCmdBuffer = CVulkanRenderManager::GetRootCommandBuffer( frameIndex );
			renderInfo.ImageIndex = CStaticRenderer::GetCurrentImageIndex();

			presenter->TransitionRender( &renderInfo );
		}
	};

	void CVulkanRenderManager::EndRootCommandBuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		vkEndCommandBuffer( m_RootFrameData[frameIndex].CommandBuffer );
	};

	void CVulkanRenderManager::SubmitRootCommandBuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice();
		if ( m_RootFrameData[frameIndex].InFlightFence )
			vkWaitForFences( *device->GetDevice(), 1, &m_RootFrameData[frameIndex].InFlightFence, VK_TRUE, UINT64_MAX );

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = m_RootFrameData[frameIndex].CommandBuffer;
		

		VkSemaphoreSubmitInfo waitSemaphoreInfo{};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		waitSemaphoreInfo.semaphore = m_RootFrameData[frameIndex].ImageAvailableSemaphore;

		VkSemaphoreSubmitInfo signalSemaphoreInfo{};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		signalSemaphoreInfo.semaphore = m_RootFrameData[frameIndex].RenderFinishedSemaphore;


		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1; 
		submitInfo.pWaitSemaphoreInfos =  &waitSemaphoreInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

		vkResetFences( *device->GetDevice(), 1, &m_RootFrameData[frameIndex].InFlightFence );
		MW_VK_CHECK( vkQueueSubmit2( *device->GetGraphicsQueue(), 1, &submitInfo, m_RootFrameData[frameIndex].InFlightFence ), "Failed to submit Draw Commandbuffers" );

	}

	void CVulkanRenderManager::BeginWorkerCommandBuffers( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		for ( auto& workerData : m_WorkerRenderData )
		{
			vkBeginCommandBuffer( workerData.CommandBuffers[frameIndex], &beginInfo );
		}
	};

	void CVulkanRenderManager::EndWorkerCommandBuffers( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// batching worker command buffers for optimal submission
		static std::vector<VkCommandBuffer> workerCommandBuffers;
		workerCommandBuffers.reserve( m_WorkerRenderData.size() );
			
		for ( auto& workerData : m_WorkerRenderData )
		{
			workerCommandBuffers.push_back( workerData.CommandBuffers[frameIndex] );
		}

		if ( !workerCommandBuffers.empty() )
			vkCmdExecuteCommands( m_RootFrameData[frameIndex].CommandBuffer, workerCommandBuffers.size(), workerCommandBuffers.data() );

		for ( auto& workerData : m_WorkerRenderData )
		{
			vkEndCommandBuffer( workerData.CommandBuffers[frameIndex] );
		}
	};


}
