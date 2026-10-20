#include <mwpch.hh>

#include <fstream>

#include <rhi/specific/vulkan/VulkanRenderManager.hh>
#include <rhi/specific/vulkan/VulkanContext.hh>
#include <rhi/specific/vulkan/VulkanPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>
#include <rhi/specific/vulkan/VulkanGraphicsPipeline.hh>
#include <rhi/specific/vulkan/VulkanVertexBuffer.hh>
#include <rhi/specific/vulkan/VulkanIndexBuffer.hh>
#include <rhi/specific/vulkan/VulkanComputePipeline.hh>

#include <rhi/agnostic/IndexBuffer.hh>

#include <renderer/FrameGraph.hh>
#include <renderer/StaticRenderer.hh>

#include <core/Application.hh>

#include "VulkanRenderer.hh"

namespace Monoworks::RHI
{
	void CVulkanRenderer::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CVulkanRenderer" );

#ifdef MW_ENABLE_MANUAL_RENDERDOC
#ifdef MW_PLATFORM_WINDOWS
		if ( HMODULE mod = GetModuleHandleA( "renderdoc.dll" ) )
		{
			pRENDERDOC_GetAPI rd_GetAPI = ( pRENDERDOC_GetAPI )GetProcAddress( mod, "RENDERDOC_GetAPI" );
			rd_GetAPI( eRENDERDOC_API_Version_1_1_2, ( void** )&m_RenderDocAPI );
		}
#else
		if ( void* mod = dlopen( "librenderdoc.so", RTLD_NOW | RTLD_NOLOAD ) )
		{
			pRENDERDOC_GetAPI rd_GetAPI = ( pRENDERDOC_GetAPI )dlsym( mod, "RENDERDOC_GetAPI" );
			int ret = rd_GetAPI( eRENDERDOC_API_Version_1_1_2, ( void** )&m_RenderDocAPI );
		}
#endif
#endif


	}

	void CVulkanRenderer::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		WaitRenderingIdle();
		CVulkanRenderManager::Shutdown();
		MW_INFO( "Shutdown CVulkanRenderer" );
	}

	 void CVulkanRenderer::WaitRenderingIdle() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		vkDeviceWaitIdle(*CVulkanContext::GetDevice());
	};

	NODISCARD static VkAttachmentLoadOp ToVulkanLoadOp( 
		EAttachmentLoadOp loadOp ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch ( loadOp )
		{
		case MW_ATTACHMENT_LOAD_OP_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case MW_ATTACHMENT_LOAD_OP_DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case MW_ATTACHMENT_LOAD_OP_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
		default:
			MW_API_WARN( "Pass invalid EAttachmentLoadOp.");
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		
		}
	}

	NODISCARD static VkAttachmentStoreOp ToVulkanStoreOp( 
		EAttachmentStoreOp storeOp ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		switch ( storeOp )
		{
		case MW_ATTACHMENT_STORE_OP_STORE: return VK_ATTACHMENT_STORE_OP_STORE;
		case MW_ATTACHMENT_STORE_OP_DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default:
			MW_API_WARN( "Pass invalid EAttachmentStoreOp." );
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
	}

	NODISCARD static VkResolveModeFlagBits ToVulkanResolveMode( 
		EResolveMode resolveMode ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// TODO: redo
		switch ( resolveMode )
		{
		case MW_RESOLVE_MODE_AVERAGE_BIT: return VK_RESOLVE_MODE_AVERAGE_BIT;
		case MW_RESOLVE_MODE_MAXIMUM_BIT: return VK_RESOLVE_MODE_MAX_BIT;
		case MW_RESOLVE_MODE_MINIMUM_BIT: return VK_RESOLVE_MODE_MIN_BIT;
		case MW_RESOLVE_MODE_SAMPLE_ZERO_BIT: return VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
		case MW_RESOLVE_MODE_NONE: return VK_RESOLVE_MODE_NONE;
		default:
			MW_API_WARN( "Pass invalid EResolveModeBits.");
			return VK_RESOLVE_MODE_NONE;
		}
	}

	void CVulkanRenderer::BeginRendering( 
		u32 frameIndex,
		const BeginRenderingInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto cmd = *CVulkanRenderManager::GetCurrentRootGraphicsCommandBuffer();
		auto width = CApplication::GetCreateInfos()->RenderableExtent.Width;
		auto height = CApplication::GetCreateInfos()->RenderableExtent.Height;


		std::vector<VkRenderingAttachmentInfo> colorAttachments;
		{
			if ( pInfo->pColorAttachments )
			{
				colorAttachments.resize( pInfo->pColorAttachments->size() );
				auto i{ 0uz };
				for ( auto& ra : colorAttachments )
				{
					auto colorAttInfo = pInfo->pColorAttachments.value()[i];
					auto image = colorAttInfo.hAttachmentImage.As<CVulkanTexture2D>();
					auto resolveImage = colorAttInfo.hResolveImage.As<CVulkanTexture2D>();

					ra.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					ra.clearValue = { .color = { 1.0f, 1.0f, 1.0f } };
					ra.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					ra.imageView = *image->GetImageView();
					ra.imageLayout = ( VkImageLayout )image->m_Layout;

					ra.loadOp = ToVulkanLoadOp( colorAttInfo.LoadOp );
					ra.storeOp = ToVulkanStoreOp( colorAttInfo.StoreOp );

					if ( colorAttInfo.hResolveImage )
					{
						ra.resolveImageView = *resolveImage->GetImageView();
						ra.resolveImageLayout = ( VkImageLayout )resolveImage->m_Layout;
						ra.resolveMode = ToVulkanResolveMode( colorAttInfo.ResolveMode );
					}

					i++;
				}
			}
		}

		VkRenderingAttachmentInfo depthAttachment{};

		if ( pInfo->pDepthAttachment )
		{
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.clearValue = { .depthStencil = { 1.0f, 0 } };
			depthAttachment.imageLayout = ( VkImageLayout )pInfo->pDepthAttachment->hAttachmentImage->m_Layout;
			depthAttachment.imageView = *pInfo->pDepthAttachment->hAttachmentImage.As<CVulkanTexture2D>()->GetImageView();
			

			depthAttachment.loadOp = ToVulkanLoadOp( pInfo->pDepthAttachment->LoadOp);
			depthAttachment.storeOp = ToVulkanStoreOp( pInfo->pDepthAttachment->StoreOp);

			if ( pInfo->pDepthAttachment->hResolveImage )
			{
				depthAttachment.resolveImageView = *pInfo->pDepthAttachment->hResolveImage.As<CVulkanTexture2D>()->GetImageView();
				depthAttachment.resolveImageLayout = ( VkImageLayout )pInfo->pDepthAttachment->hResolveImage->m_Layout;
				depthAttachment.resolveMode = ToVulkanResolveMode( pInfo->pDepthAttachment->ResolveMode );
			}
		}

		VkRenderingAttachmentInfo stencilAttachment{};

		if ( pInfo->pStencilAttachment )
		{
			stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			stencilAttachment.clearValue = { .depthStencil = { 1.0f, 0 } };
			stencilAttachment.imageLayout = ( VkImageLayout )pInfo->pStencilAttachment->hAttachmentImage->m_Layout;
			stencilAttachment.imageView = *pInfo->pStencilAttachment->hAttachmentImage.As<CVulkanTexture2D>()->GetImageView();

			stencilAttachment.loadOp = ToVulkanLoadOp( pInfo->pStencilAttachment->LoadOp );
			stencilAttachment.storeOp = ToVulkanStoreOp( pInfo->pStencilAttachment->StoreOp );

			if ( pInfo->pStencilAttachment->hResolveImage )
			{
				stencilAttachment.resolveImageView = *pInfo->pStencilAttachment->hResolveImage.As<CVulkanTexture2D>()->GetImageView();
				stencilAttachment.resolveImageLayout = ( VkImageLayout )pInfo->pStencilAttachment->hResolveImage->m_Layout;
				stencilAttachment.resolveMode = ToVulkanResolveMode( pInfo->pStencilAttachment->ResolveMode );
			}
		}
		
		VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT | VK_RENDERING_CONTENTS_INLINE_BIT_KHR;
		renderingInfo.renderArea.extent = { pInfo->RenderArea.Width, pInfo->RenderArea.Height };
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.layerCount = 1;
        
		renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.empty() ? nullptr : colorAttachments.data();
		if ( pInfo->pDepthAttachment )
			renderingInfo.pDepthAttachment = &depthAttachment;
		if ( pInfo->pStencilAttachment )
			renderingInfo.pStencilAttachment = &stencilAttachment;

        vkCmdBeginRendering( cmd, &renderingInfo );

    };

    void CVulkanRenderer::EndRendering( 
		u32 frameIndex ) NOEXCEPT
    {
        MW_PROFILE_FUNC;

        auto cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

        vkCmdEndRendering( cmd );

    }

	 void CVulkanRenderer::DispatchCompute( 
		u32 frameIndex,
		Ref<IComputePipeline> hPipeline,
		Vector workgroup,
		std::span<DescriptorHandle> ppDesciptors,
		u32 firstSet,
		s32 MW_NULLABLE threadID /*= -1*/) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		VkCommandBuffer hCmd = nullptr;

		if ( threadID < 0 )
			hCmd = *CVulkanRenderManager::GetCurrentRootGraphicsCommandBuffer();
		else
			hCmd = *CVulkanRenderManager::GetCurrentWorkerGraphicsCommandBuffer( threadID );

		if ( !hCmd )
		{
			MW_ERROR( "Failed to get current worker or root command buffer. Discarding Dispatch" );
			return;
		}

		auto vkPipeline = hPipeline.As<CVulkanComputePipeline>();
        vkCmdBindDescriptorSets( hCmd, VK_PIPELINE_BIND_POINT_COMPUTE, *vkPipeline->GetVulkanPipelineSignature(), firstSet, ppDesciptors.size(), (VkDescriptorSet*)(ppDesciptors.data()), 0, nullptr);

		vkCmdBindPipeline( hCmd, VK_PIPELINE_BIND_POINT_COMPUTE, *vkPipeline->GetVulkanPipeline() );

		vkCmdDispatch( hCmd, workgroup.x, workgroup.y, workgroup.z );
	}

	 void CVulkanRenderer::DispatchCompute2( 
		u32 frameIndex, 
		Ref<IComputePipeline> hPipeline,
		Vector workgroup,
		s32 MW_NULLABLE threadID /*= -1 */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		VkCommandBuffer hCmd = nullptr;

		if ( threadID < 0 )
			hCmd = *CVulkanRenderManager::GetCurrentRootGraphicsCommandBuffer();
		else
			hCmd = *CVulkanRenderManager::GetCurrentWorkerGraphicsCommandBuffer( threadID );

		if ( !hCmd )
		{
			MW_ERROR( "Failed to get current worker or root command buffer. Discarding Dispatch" );
			return;
		}

		auto vkPipeline = hPipeline.As<CVulkanComputePipeline>();
		vkCmdBindPipeline( hCmd, VK_PIPELINE_BIND_POINT_COMPUTE, *vkPipeline->GetVulkanPipeline() );

		vkCmdDispatch( hCmd, workgroup.x, workgroup.y, workgroup.z );

	}

	 void CVulkanRenderer::ProfileFrameData() NOEXCEPT
	{
        MW_PROFILE_FUNC;
#ifdef MW_PROFILING
		auto& total = CVulkanContext::GetTotalVulkanAllocations();
		MW_PROFILE_PLOT( "Vulkan Command Allocations", ( s64 )total.CommandAllocs );
		MW_PROFILE_PLOT( "Vulkan Object Allocations", ( s64 )total.ObjectAllocs );
		MW_PROFILE_PLOT( "Vulkan Cache Allocations", ( s64 )total.ObjectAllocs );
		MW_PROFILE_PLOT( "Vulkan Device Allocations", ( s64 )total.DeviceAllocs );
		MW_PROFILE_PLOT( "Vulkan Instance Allocations", ( s64 )total.InstanceAllocs );
#endif
	}

	 u32 CVulkanRenderer::AcquireNextImage( 
		u32 frameIndex ) NOEXCEPT
	{
        MW_PROFILE_FUNC;
        auto presenter = CVulkanContext::GetPresenter();

		if ( CApplication::GetCreateInfos()->UseSDL && CApplication::GetCreateInfos()->UseSwapchain )
		{
			SVulkanSDLPresentationAcquisitionInfo acquisitionInfo{};
			acquisitionInfo.pDevice = CVulkanContext::GetDevice()->GetDevice();
			acquisitionInfo.pPhysDevice = CVulkanContext::GetDevice()->GetPhysicalDevice();
			acquisitionInfo.pVulkanDevice = CVulkanContext::GetDevice();
			acquisitionInfo.pImageAvailableSemaphore = CVulkanRenderManager::GetImageAvailableSemaphore( frameIndex );
			acquisitionInfo.pInFlightFence = CVulkanRenderManager::GetInFlightFence( frameIndex );

			return presenter->Acquire( &acquisitionInfo );
		}
		else if ( CApplication::GetCreateInfos()->UseQt )
		{
			SVulkanQtPresentationAcquisitionInfo acquisitionInfo{};
			acquisitionInfo.pGraphicsQueue = CVulkanContext::GetDevice()->GetGraphicsQueue();
			acquisitionInfo.pImageAvailableSemaphore = CVulkanRenderManager::GetImageAvailableSemaphore( frameIndex );
			acquisitionInfo.pQtReadFinishedSemaphore = CVulkanRenderManager::GetQtReadFinishedSemaphore( frameIndex );
			acquisitionInfo.pInFlightFence = CVulkanRenderManager::GetInFlightFence( frameIndex );
			acquisitionInfo.pVulkanDevice = CVulkanContext::GetDevice();

			return presenter->Acquire( &acquisitionInfo );
		}
	}

	 void CVulkanRenderer::Present( 
		MAYBE_UNUSED u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto presenter = CVulkanContext::GetPresenter();

		if ( CApplication::GetCreateInfos()->UseSDL && CApplication::GetCreateInfos()->UseSwapchain )
		{
			SVulkanSDLPresentationPresentInfo presentInfo{};
			presentInfo.pDevice = CVulkanContext::GetDevice()->GetDevice();
			presentInfo.pPhysDevice = CVulkanContext::GetDevice()->GetPhysicalDevice();
			presentInfo.pVulkanDevice = CVulkanContext::GetDevice();
			presentInfo.pRenderFinishedSemaphore = CVulkanRenderManager::GetPresentWaitSemaphore( CFrameManager::GetCurrentImageIndex() );
			presentInfo.pPresentQueue = CVulkanContext::GetDevice()->GetPresentQueue();
			presentInfo.pImageIndex = &CFrameManager::GetCurrentImageIndex();

			return presenter->Present( &presentInfo );
		}
		else if ( CApplication::GetCreateInfos()->UseQt )
		{
			SVulkanQtPresentationPresentInfo presentInfo{};
			presentInfo.pImageIndex = &CFrameManager::GetCurrentImageIndex();

			return presenter->Present( &presentInfo );
		}
	}

	 void CVulkanRenderer::BindGraphicsPipeline( 
		u32 frameIndex, 
		Ref<IGraphicsPipeline> hPipeline,
		s32 MW_NULLABLE threadID /*= -1 */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex >= MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding graphics pipeline bind on threadID {}.", threadID );
			return;
		}

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
		{
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding graphics pipeline bind on threadID {}.", threadID, threadID );
			return;
		}

		auto vkPipeline = hPipeline.As<CVulkanGraphicsPipeline>();

		vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *vkPipeline->GetVulkanPipeline() );
	}

	 void CVulkanRenderer::BindDescriptors( 
		u32 frameIndex,
		PipelineSignature pSignature,
		std::span<DescriptorHandle> ppDescriptors,
		u32 firstSet,
		s32 MW_NULLABLE threadID /*= -1 */,
		RHI::EPipelineBindPoint bindpoint )
	{
		MW_PROFILE_FUNC;
		
		if ( frameIndex >= MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding descriptor bind on threadID {}.", threadID );
			return;
		}

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
		{
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding descriptor bind on threadID {}.", threadID, threadID );
			return;
		}

		// Bind descriptors for graphics
		switch ( bindpoint )
		{
		case MW_PIPELINE_BIND_POINT_COMPUTE:
			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ( VkPipelineLayout )pSignature, firstSet, ppDescriptors.size(), ( VkDescriptorSet* )ppDescriptors.data(), 0, nullptr);
			break;
		case MW_PIPELINE_BIND_POINT_GRAPHICS:
			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ( VkPipelineLayout )pSignature, firstSet, ppDescriptors.size(), ( VkDescriptorSet* )ppDescriptors.data(), 0, nullptr);
			break;
		case MW_PIPELINE_BIND_POINT_BOTH:
			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ( VkPipelineLayout )pSignature, firstSet, ppDescriptors.size(), ( VkDescriptorSet* )ppDescriptors.data(), 0, nullptr);
			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ( VkPipelineLayout )pSignature, firstSet, ppDescriptors.size(), ( VkDescriptorSet* )ppDescriptors.data(), 0, nullptr);
			break;
		default:
			MW_API_ERROR("Invalid EPipelineBindPoint passed");
		}

	}

	 void CVulkanRenderer::SetDynamicViewports( 
		u32 frameIndex, 
		std::span<const Viewport> ppViewports,
		size_t firstViewport ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		
		if ( frameIndex >= MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic viewport state bind." );
			return;
		}

		if ( ppViewports.empty() )
		{
			MW_API_ERROR( "ppViewports is empty. Discarding dynamic viewport state bind." );
			return;
		}

		if ( ppViewports.size() <= 0 )
		{
			MW_API_ERROR( "ppViewports.size() must be greater than 0. Discarding dynamic viewport state bind." );
			return;
		}


		const auto& workerData = CVulkanRenderManager::GetWorkerFrameData();
		VkCommandBuffer rootCmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

		// Binds the viewports for the root command buffer.
		vkCmdSetViewport( rootCmd, ( u32 )firstViewport, ( u32 )ppViewports.size(), ( VkViewport* )ppViewports.data() );
	
		// Binds the viewport for all worker command buffers since state isn't inherited from the primary command buffer. 
		for ( auto& worker : workerData )
			vkCmdSetViewport( worker.GraphicsCommandBuffers[frameIndex], ( u32 )firstViewport, ( u32 )ppViewports.size(), ( VkViewport* )ppViewports.data() );
	}

	 void CVulkanRenderer::SetDynamicScissors( 
		u32 frameIndex,
		std::span<const SExtent2D> ppScissors,
		size_t firstScissor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind." );
			return;
		}

		if ( ppScissors.empty() )
		{
			MW_API_ERROR( "ppScissors is empty. Discarding dynamic scissor state bind." );
			return;
		}

		if ( ppScissors.size() <= 0 )
		{
			MW_API_ERROR( "ppScissors.size() must be greater than 0. Discarding dynamic scissor state bind." ); 
			return;
		}

		std::vector<VkRect2D> scissors( ppScissors.size()  );

		auto i{ 0uz };
		for ( VkRect2D& scissor : scissors )
		{
			scissor.extent = { ppScissors[i].Width, ppScissors[i].Height };
			scissor.offset = {};
			i++;
		}
		const auto& workerData = CVulkanRenderManager::GetWorkerFrameData();
		VkCommandBuffer rootCmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

		vkCmdSetScissor( rootCmd, firstScissor, ppScissors.size(), scissors.data() );
			
		for ( auto& worker : workerData )
			vkCmdSetScissor( worker.GraphicsCommandBuffers[frameIndex], firstScissor, ppScissors.size(), scissors.data() );

	}


	NODISCARD static  VkCullModeFlagBits ToVulkanCullMode( 
		ECullMode mode ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		switch ( mode )
		{
		case MW_CULL_MODE_NONE:
			return VK_CULL_MODE_NONE;
		case MW_CULL_MODE_FRONT:
			return VK_CULL_MODE_FRONT_BIT;
		case MW_CULL_MODE_BACK:
			return VK_CULL_MODE_BACK_BIT;
		case MW_CULL_MODE_FRONT_AND_BACK:
			return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			MW_API_ERROR( "Pass invalid ECullMode Enumeration." );
			return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
		}
	};
	

	 void CVulkanRenderer::SetDynamicCullMode( 
		u32 frameIndex,
		ECullMode cullMode ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind." );
			return;
		}
		
		const auto& workerData = CVulkanRenderManager::GetWorkerFrameData();
		VkCommandBuffer rootCmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

		const auto cum = ToVulkanCullMode( cullMode );

		vkCmdSetCullMode( rootCmd, cum );

		for ( auto& worker : workerData )
			vkCmdSetCullMode( worker.GraphicsCommandBuffers[frameIndex], cum );

	}


	 void CVulkanRenderer::SetDynamicViewportsST( 
		u32 frameIndex,
		std::span<const Viewport> ppViewports,
		size_t firstViewport,
		s32 threadID ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind on threadID {}.", threadID );
			return;
		}

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
		{
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding dynamic scissor state bind on threadID {}.", threadID, threadID );
			return;
		}

		vkCmdSetViewport( cmd, firstViewport, ppViewports.size(), ( VkViewport* )ppViewports.data());
	}

	 void CVulkanRenderer::SetDynamicScissorsST( 
		u32 frameIndex,
		std::span<const SExtent2D> ppScissors,
		size_t firstScissor,
		s32 threadID ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		

		if ( frameIndex > MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind on threadID {}.", threadID );
			return;
		}

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
		{
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding dynamic scissor state bind on threadID {}.", threadID, threadID );
			return;
		}

		std::vector<VkRect2D> scissors( ppScissors.size() );
		auto i{ 0uz };
		for ( VkRect2D& scissor : scissors )
		{
			scissor.extent = { ppScissors[i].Width, ppScissors[i].Height };
			scissor.offset = {};
			i++;
		}

		vkCmdSetScissor( cmd, firstScissor, ppScissors.size(), scissors.data() );

	}

	 void CVulkanRenderer::SetDynamicCullModeST( 
		u32 frameIndex,
		ECullMode cullMode,
		s32 threadID ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
		{
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind on threadID {}.", threadID );
			return;
		}

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
		{
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding dynamic scissor state bind on threadID {}.", threadID, threadID ); 
			return;
		}

		vkCmdSetCullMode( cmd, ToVulkanCullMode( cullMode ) );
	}

	 void CVulkanRenderer::BeginRootCommandbuffer( 
		u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::BeginRootGraphicsCommandBuffer( frameIndex );
	}

	void CVulkanRenderer::SubmitRootCommandbuffer( 
		u32 frameIndex  )
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::EndRootGraphicsCommandBuffer( frameIndex );
		CVulkanRenderManager::SubmitRootGraphicsCommandBuffer( frameIndex );
	}

	 void CVulkanRenderer::BeginSecondaryCommandbuffers( 
		u32 frameIndex  ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::BeginWorkerGraphicsCommandBuffers( frameIndex );
	}

	 void CVulkanRenderer::MergeSecondaryCommandbuffers(
		u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::EndWorkerGraphicsCommandBuffers( frameIndex );
	}

	 void CVulkanRenderer::DrawStaticMeshIndexed( 
		u32 frameIndex,
		Ref<CMesh> hMesh,
		const Matrix& transform, 
		s32 threadID /* = -1 */,
		Ref<IGraphicsPipeline> hPipelineOverride /* = nullptr */) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto vkVBuf = hMesh->VertexBuffer.As<CVulkanVertexBuffer>()->GetVulkanBuffer();
		auto vkIBuf = hMesh->IndexBuffer.As<CVulkanIndexBuffer>()->GetVulkanBuffer();
		
		VkCommandBuffer cmd = nullptr;
		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		const bool useMaterialPipeline = !hPipelineOverride;
		auto mat = hMesh->Material;

		Ref<IGraphicsPipeline> hPipeline = useMaterialPipeline ? mat->GetPipeline() : hPipelineOverride;
		auto vkGPipe = hPipeline.As<CVulkanGraphicsPipeline>();

		vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *vkGPipe->GetVulkanPipeline() );

		VkDeviceSize offset = 0; 
		vkCmdBindVertexBuffers( cmd, 0, 1, vkVBuf, &offset );
		vkCmdBindIndexBuffer( cmd, *vkIBuf, 0, VK_INDEX_TYPE_UINT32	);

		CDefferedFrameGraph::ModelPushConstant modelConstants;
		modelConstants.CurrentModelMatrix = transform;
		modelConstants.PreviousModelMatrix = transform;
		modelConstants.EntityID = 1;

		constexpr u32 modelPCSize = sizeof( CDefferedFrameGraph::ModelPushConstant );

		// Make multiple of 4
		vkCmdPushConstants( cmd, *vkGPipe->GetVulkanPipelineSignature(), VK_SHADER_STAGE_ALL, 0, modelPCSize, &modelConstants );

		if ( useMaterialPipeline )
		{
			auto descriptors = mat->GetDescriptors();

			const auto& frameDescriptors = descriptors[frameIndex];

			bool isFirstInvalid = !frameDescriptors.empty() && frameDescriptors[0] == nullptr;
			const u32 firstSet = isFirstInvalid ? 1u : 0u;

			auto descView = frameDescriptors | std::views::drop( isFirstInvalid ? 1 : 0 );

			auto setCount = static_cast< uint32_t >( std::ranges::size( descView ) );

			if ( setCount > 0 )
			{
				vkCmdBindDescriptorSets(
					cmd,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					*vkGPipe->GetVulkanPipelineSignature(),
					firstSet,
					setCount,
					reinterpret_cast< const VkDescriptorSet* >( std::ranges::data( descView ) ),
					0, nullptr
				);
			}
		}

		vkCmdDrawIndexed( cmd, hMesh->IndexBuffer->GetCount(), 1, 0, 0, 0);
		CStaticRenderer::GetFrameGraphicsProfilingData()->DrawCallCount++;
	}

}