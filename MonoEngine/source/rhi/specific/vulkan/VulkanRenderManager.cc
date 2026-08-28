#include <mwpch.hh>

#include <core/Application.hh>
#include <renderer/StaticRenderer.hh>

#include <rhi/specific/vulkan/VulkanContext.hh>
#include <rhi/specific/vulkan/VulkanPresenter.hh>

#ifdef MW_ENABLE_MANUAL_RENDERDOC
#include <renderdoc_app.h>
#endif

#ifdef MW_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "VulkanRenderManager.hh"


namespace Monoworks::RHI 
{
	SVulkanFrameData			   CVulkanRenderManager::m_RootFrameData[MFIF];
	std::vector<SVulkanWorkerData> CVulkanRenderManager::m_WorkerRenderData;

	void CVulkanRenderManager::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CVulkanRenderManager" );
		
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
			
			MW_VK_CHECK( vkCreateSemaphore( *device->GetDevice(), &semaphoreCreateInfo, CVulkanContext::GetCallbacks(), &frameData.QtReadFinishedSemaphore ), "Failed to create QtReadFinishedSemaphore." );
			MW_VK_CHECK( vkCreateSemaphore( *device->GetDevice(), &semaphoreCreateInfo, CVulkanContext::GetCallbacks(), &frameData.ImageAvailableSemaphore ), "Failed to create ImageAvailableSemaphore." );
			MW_VK_CHECK( vkCreateSemaphore( *device->GetDevice(), &semaphoreCreateInfo, CVulkanContext::GetCallbacks(), &frameData.GraphicsSubmitSemaphore ), "Failed to create GraphicsSubmitSemaphore." );
			MW_VK_CHECK( vkCreateSemaphore( *device->GetDevice(), &semaphoreCreateInfo, CVulkanContext::GetCallbacks(), &frameData.RenderFinishedSemaphore ), "Failed to create RenderFinishedSemaphore." );
			MW_VK_CHECK( vkCreateFence( *device->GetDevice(), &fenceCreateInfo, CVulkanContext::GetCallbacks(), &frameData.InFlightFence), "Failed to create InFlightFence.");

			{
				VkCommandPoolCreateInfo poolCreateInfo{};
				poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolCreateInfo.queueFamilyIndex = device->GetGraphicsQueueFamilyIndex();
				poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				MW_VK_CHECK( vkCreateCommandPool( *device->GetDevice(), &poolCreateInfo, CVulkanContext::GetCallbacks(), &frameData.GraphicsCommandPool ), "Failed to create graphics CommandPool." );

				VkCommandBufferAllocateInfo allocCreateInfo{};
				allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocCreateInfo.commandPool = frameData.GraphicsCommandPool;
				allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocCreateInfo.commandBufferCount = 1;

				MW_VK_CHECK( vkAllocateCommandBuffers( *device->GetDevice(), &allocCreateInfo, &frameData.GraphicsCommandBuffer ), "Failed to allocate graphics CommandBuffer." );
			}

			{
				VkCommandPoolCreateInfo poolCreateInfo{};
				poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolCreateInfo.queueFamilyIndex = device->GetComputeQueueFamilyIndex();
				poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				MW_VK_CHECK( vkCreateCommandPool( *device->GetDevice(), &poolCreateInfo, CVulkanContext::GetCallbacks(), &frameData.ComputeCommandPool ), "Failed to create compute CommandPool." );

				VkCommandBufferAllocateInfo allocCreateInfo{};
				allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocCreateInfo.commandPool = frameData.ComputeCommandPool;
				allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocCreateInfo.commandBufferCount = 1;

				MW_VK_CHECK( vkAllocateCommandBuffers( *device->GetDevice(), &allocCreateInfo, &frameData.ComputeCommandBuffer ), "Failed to allocate compute CommandBuffer." );
			}
		}

		for ( auto& workerData : m_WorkerRenderData )
		{
			// Graphics command buffer creation
			for ( u32 i{0}; i < MFIF; i++ )
			{
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = device->GetGraphicsQueueFamilyIndex();
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				MW_VK_CHECK( vkCreateCommandPool( *device->GetDevice(), &poolInfo, CVulkanContext::GetCallbacks(), &workerData.GraphicsCommandPools[i] ), "Failed to create graphics CommandPool.");
			
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = workerData.GraphicsCommandPools[i];
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				allocInfo.commandBufferCount = 1;

				MW_VK_CHECK( vkAllocateCommandBuffers( *device->GetDevice(), &allocInfo, &workerData.GraphicsCommandBuffers[i] ), "Failed to allocate graphics CommandBuffer.");
			}
			// Compute command buffer creation
			for ( u32 i{ 0 }; i < MFIF; i++ )
			{
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = device->GetComputeQueueFamilyIndex();
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				MW_VK_CHECK( vkCreateCommandPool( *device->GetDevice(), &poolInfo, CVulkanContext::GetCallbacks(), &workerData.ComputeCommandPools[i] ), "Failed to create compute CommandPool." );

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = workerData.ComputeCommandPools[i];
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				allocInfo.commandBufferCount = 1;

				MW_VK_CHECK( vkAllocateCommandBuffers( *device->GetDevice(), &allocInfo, &workerData.ComputeCommandBuffers[i] ), "Failed to allocate compute CommandBuffer." );
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
			for ( auto& commandPool : workerData.GraphicsCommandPools )
			{
				if ( commandPool )
					vkDestroyCommandPool( device, commandPool, CVulkanContext::GetCallbacks() );
			}
			
			for ( auto& commandPool : workerData.ComputeCommandPools )
			{
				if ( commandPool )
					vkDestroyCommandPool( device, commandPool, CVulkanContext::GetCallbacks() );
			}
		}

		m_WorkerRenderData.clear();

		for ( auto& frameData : m_RootFrameData )
		{
			if ( frameData.ImageAvailableSemaphore )
				vkDestroySemaphore( device, frameData.ImageAvailableSemaphore, CVulkanContext::GetCallbacks() );
			
			if ( frameData.RenderFinishedSemaphore )
				vkDestroySemaphore( device, frameData.RenderFinishedSemaphore, CVulkanContext::GetCallbacks() );

			if ( frameData.QtReadFinishedSemaphore )
				vkDestroySemaphore( device, frameData.QtReadFinishedSemaphore, CVulkanContext::GetCallbacks() );


			if ( frameData.RenderFinishedSemaphore )
				vkDestroySemaphore( device, frameData.RenderFinishedSemaphore, CVulkanContext::GetCallbacks() );

			if ( frameData.InFlightFence )
				vkDestroyFence( device, frameData.InFlightFence, CVulkanContext::GetCallbacks() );

			if ( frameData.GraphicsCommandPool )
				vkDestroyCommandPool( device, frameData.GraphicsCommandPool, CVulkanContext::GetCallbacks() );

			if ( frameData.ComputeCommandPool )
				vkDestroyCommandPool( device, frameData.ComputeCommandPool, CVulkanContext::GetCallbacks() );
		}
		MW_INFO( "Shutdown CVulkanRenderManager" );
	};

	void CVulkanRenderManager::BeginRootGraphicsCommandBuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;


		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer( m_RootFrameData[frameIndex].GraphicsCommandBuffer, &beginInfo );


		auto presenter = CVulkanContext::GetPresenter();
		if ( CApplication::GetCreateInfos()->UseSDL )
		{
			SVulkanSDLPresentationTransitionRenderInfo renderInfo{};
			renderInfo.pCmdBuffer = CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
			renderInfo.ImageIndex = CStaticRenderer::GetCurrentImageIndex();

			presenter->TransitionRender( &renderInfo );
		} 
		else if ( CApplication::GetCreateInfos()->UseQt )
		{
			SVulkanQtPresentationTransitionRenderInfo renderInfo {};
			renderInfo.pCmdBuffer = CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
			renderInfo.ImageIndex = CStaticRenderer::GetCurrentImageIndex();

			presenter->TransitionRender( &renderInfo );
		}
	};

	void CVulkanRenderManager::EndRootGraphicsCommandBuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		vkEndCommandBuffer( m_RootFrameData[frameIndex].GraphicsCommandBuffer );

	};

	void CVulkanRenderManager::SubmitRootGraphicsCommandBuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice();

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = m_RootFrameData[frameIndex].GraphicsCommandBuffer;
		
		VkSemaphoreSubmitInfo waitSemaphoreInfo{};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		waitSemaphoreInfo.semaphore = m_RootFrameData[frameIndex].ImageAvailableSemaphore;

		VkSemaphoreSubmitInfo signalSemaphoreInfo{};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		signalSemaphoreInfo.semaphore = m_RootFrameData[frameIndex].GraphicsSubmitSemaphore;


		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1; 
		submitInfo.pWaitSemaphoreInfos =  &waitSemaphoreInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

		MW_VK_CHECK( vkQueueSubmit2( *device->GetGraphicsQueue(), 1, &submitInfo, nullptr ), "Failed to submit Graphics Commandbuffers" );

	}

	void CVulkanRenderManager::BeginWorkerGraphicsCommandBuffers( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		for ( auto& workerData : m_WorkerRenderData )
		{
			vkBeginCommandBuffer( workerData.GraphicsCommandBuffers[frameIndex], &beginInfo );
		}
	};

	void CVulkanRenderManager::EndWorkerGraphicsCommandBuffers( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// batching worker command buffers for optimal submission
		static std::vector<VkCommandBuffer> workerCommandBuffers;
		workerCommandBuffers.clear();
		workerCommandBuffers.reserve( m_WorkerRenderData.size() );

		for ( auto& workerData : m_WorkerRenderData )
		{
			VkCommandBuffer cmd = workerData.GraphicsCommandBuffers[frameIndex];

			if ( std::find( workerCommandBuffers.begin(), workerCommandBuffers.end(), cmd ) == workerCommandBuffers.end() )
			{
				workerCommandBuffers.push_back( cmd );
			}
		}

		if ( !workerCommandBuffers.empty() )
			vkCmdExecuteCommands( m_RootFrameData[frameIndex].GraphicsCommandBuffer, ( u32 )workerCommandBuffers.size(), workerCommandBuffers.data() );

		for ( auto& workerData : m_WorkerRenderData )
		{
			vkEndCommandBuffer( workerData.GraphicsCommandBuffers[frameIndex] );
		}
	};

	void CVulkanRenderManager::BeginRootComputeCommandBuffer( u32 frameIndex )		NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer( m_RootFrameData[frameIndex].ComputeCommandBuffer, &beginInfo );

	};

	void CVulkanRenderManager::EndRootComputeCommandBuffer( u32 frameIndex )		NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		vkEndCommandBuffer( m_RootFrameData[frameIndex].GraphicsCommandBuffer );
	};

	void CVulkanRenderManager::SubmitRootComputeCommandBuffer( u32 frameIndex )		NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice();

		if ( m_RootFrameData[frameIndex].InFlightFence )
			vkWaitForFences( *device->GetDevice(), 1, &m_RootFrameData[frameIndex].InFlightFence, VK_TRUE, UINT64_MAX );

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = m_RootFrameData[frameIndex].ComputeCommandBuffer;

		VkSemaphoreSubmitInfo waitSemaphoreInfo{};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		waitSemaphoreInfo.semaphore = m_RootFrameData[frameIndex].GraphicsSubmitSemaphore;

		VkSemaphoreSubmitInfo signalSemaphoreInfo{};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		signalSemaphoreInfo.semaphore = m_RootFrameData[frameIndex].RenderFinishedSemaphore;


		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

		vkResetFences( *device->GetDevice(), 1, &m_RootFrameData[frameIndex].InFlightFence );
		MW_VK_CHECK( vkQueueSubmit2( *device->GetComputeQueue(), 1, &submitInfo, m_RootFrameData[frameIndex].InFlightFence ), "Failed to submit compute Commandbuffers" );

	};
		 
	void CVulkanRenderManager::BeginWorkerComputeCommandBuffers( u32 frameIndex )	NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		for ( auto& workerData : m_WorkerRenderData )
		{
			vkBeginCommandBuffer( workerData.ComputeCommandBuffers[frameIndex], &beginInfo );
		}
	};

	void CVulkanRenderManager::EndWorkerComputeCommandBuffers( u32 frameIndex )		NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		// batching worker command buffers for optimal submission
		static std::vector<VkCommandBuffer> workerCommandBuffers;
		workerCommandBuffers.clear(); 
		workerCommandBuffers.reserve( m_WorkerRenderData.size() );

		for ( auto& workerData : m_WorkerRenderData )
		{
			VkCommandBuffer cmd = workerData.ComputeCommandBuffers[frameIndex];

			if ( std::find( workerCommandBuffers.begin(), workerCommandBuffers.end(), cmd ) == workerCommandBuffers.end() )
			{
				workerCommandBuffers.push_back( cmd );
			}
		}

		if ( !workerCommandBuffers.empty() )
			vkCmdExecuteCommands( m_RootFrameData[frameIndex].ComputeCommandBuffer, ( u32 )workerCommandBuffers.size(), workerCommandBuffers.data() );

		for ( auto& workerData : m_WorkerRenderData )
		{
			vkEndCommandBuffer( workerData.ComputeCommandBuffers[frameIndex] );
		}
	};
}
