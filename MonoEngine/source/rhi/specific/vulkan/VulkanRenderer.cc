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

#include <renderer/StaticRenderer.hh>
#include <core/Application.hh>

#include "VulkanRenderer.hh"

namespace Monoworks::RHI
{
	static std::vector<char> readFile( const std::string& filename, size_t* size ) {
		std::ifstream file( filename, std::ios::ate | std::ios::binary );
		// todo remove
		if ( !file.is_open() ) {
			MW_ERROR( "LECK" );
		}

		size_t fileSize = ( size_t )file.tellg();
		*size = fileSize;
		std::vector<char> buffer( fileSize );
		file.seekg( 0 );
		file.read( buffer.data(), fileSize );
		file.close();

		return buffer;
	}

	void CVulkanRenderer::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CVulkanRenderer" );

		SShaderByteCode vertexCode{};
		auto vertextSpirv = readFile( "shaders/vertex.spirv", &vertexCode.Size );
		vertexCode.pCode = vertextSpirv.data();

		SShaderObject vertex{};
		vertex.Code = vertexCode;
		vertex.ShaderStage = MW_SHADER_STAGE_VERTEX;


		SShaderByteCode fragmentCode{};
		auto fragmentSpirv = readFile( "shaders/fragment.spirv", &fragmentCode.Size );
		fragmentCode.pCode = fragmentSpirv.data();

		SShaderObject fragment{};
		fragment.Code = fragmentCode;
		fragment.ShaderStage = MW_SHADER_STAGE_FRAGMENT;


		CVertexLayout layout
		{
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "position" }
		};

		GraphicsPipelineCreationInfo pipelineInfo{};

		pipelineInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_TEST_BIT | MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_WRITE_BIT;
		pipelineInfo.ColorFormats = { MW_FORMAT_B8G8R8A8_SRGB };
		pipelineInfo.ColorBlendAttachments = { { MW_BLEND_MODE_OPAQUE, true } };
		std::vector<SShaderObject> objects;
		objects.push_back( vertex );
		objects.push_back( fragment );
		pipelineInfo.ShaderObjects = objects;
		pipelineInfo.VertexLayout = layout;

		m_Pipeline = IGraphicsPipeline::Create( &pipelineInfo );

		std::vector<SVertex> quadVertices = {
			{ Vector( -0.5f, -0.5f, 0.5f ), },
			{ Vector( 0.5f, -0.5f, 0.5f ),  },
			{ Vector( 0.5f,  0.5f, 0.5f ),  },
			{ Vector( -0.5f,  0.5f, 0.5f ), }
		};
		m_Vertices = IVertexBuffer::Create( quadVertices.data(), ( u32 )quadVertices.size(), sizeof( SVertex ), true );

		std::vector<Index> indices = { 0, 1, 2, 2, 3, 0 };
		m_Indices = IIndexBuffer::Create( indices.data(), ( u32 )indices.size(), 0, true );

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
		CVulkanRenderManager::Shutdown();
		MW_INFO( "Shutdown CVulkanRenderer" );
	}

	NODISCARD static VkAttachmentLoadOp ToVulkanLoadOp( EAttachmentLoadOp loadOp )
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

	NODISCARD static VkAttachmentStoreOp ToVulkanStoreOp( EAttachmentStoreOp storeOp )
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

	NODISCARD static VkResolveModeFlagBits ToVulkanResolveMode( EResolveMode resolveMode )
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

	void CVulkanRenderer::BeginRendering( u32 frameIndex, const BeginRenderingInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto cmd = *CVulkanRenderManager::GetCurrentRootGraphicsCommandBuffer();
		auto width = CApplication::GetCreateInfos()->RenderableExtent.Width;
		auto height = CApplication::GetCreateInfos()->RenderableExtent.Height;

		std::vector<VkRenderingAttachmentInfo> colorAttachments( pInfo->ColorAttachmentCount );
		{
			auto i{ 0uz };
			for ( auto& ra : colorAttachments )
			{
				auto colorAttInfo = pInfo->pColorAttachments[i];
				auto image = colorAttInfo.AttachmentImage.As<CVulkanTexture2D>();
				auto resolveImage = colorAttInfo.ResolveImage.As<CVulkanTexture2D>();

				ra.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				ra.clearValue = { .color = { 1.0f, 1.0f, 1.0f } };
				ra.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				ra.imageView = *image->GetImageView();
				ra.imageLayout = ( VkImageLayout )image->Layout;
				
				ra.loadOp = ToVulkanLoadOp( colorAttInfo.LoadOp );
				ra.storeOp = ToVulkanStoreOp( colorAttInfo.StoreOp );

				if ( colorAttInfo.ResolveImage )
				{
					ra.resolveImageView = *resolveImage->GetImageView();
					ra.resolveImageLayout = ( VkImageLayout )resolveImage->Layout;
					ra.resolveMode = ToVulkanResolveMode( colorAttInfo.ResolveMode );
				}

				i++;
			}
		}

		VkRenderingAttachmentInfo depthAttachment{};

		if ( pInfo->pDepthAttachment )
		{
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.clearValue = { .color = { 1.0f, 1.0f, 1.0f } };
			depthAttachment.imageLayout = ( VkImageLayout )pInfo->pDepthAttachment->AttachmentImage->Layout;
			depthAttachment.imageView = *pInfo->pDepthAttachment->AttachmentImage.As<CVulkanTexture2D>()->GetImageView();
			
			depthAttachment.loadOp = ToVulkanLoadOp( pInfo->pDepthAttachment->LoadOp );
			depthAttachment.storeOp = ToVulkanStoreOp( pInfo->pDepthAttachment->StoreOp );

			if ( pInfo->pDepthAttachment->ResolveImage )
			{
				depthAttachment.resolveImageView = *pInfo->pDepthAttachment->ResolveImage.As<CVulkanTexture2D>()->GetImageView();
				depthAttachment.resolveImageLayout = ( VkImageLayout )pInfo->pDepthAttachment->ResolveImage->Layout;
				depthAttachment.resolveMode = ToVulkanResolveMode( pInfo->pDepthAttachment->ResolveMode );
			}
		}

		VkRenderingAttachmentInfo stencilAttachment{};

		if ( pInfo->pStencilAttachment )
		{
			stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			stencilAttachment.clearValue = { .color = { 1.0f, 1.0f, 1.0f } };
			stencilAttachment.imageLayout = ( VkImageLayout )pInfo->pStencilAttachment->AttachmentImage->Layout;
			stencilAttachment.imageView = *pInfo->pStencilAttachment->AttachmentImage.As<CVulkanTexture2D>()->GetImageView();

			stencilAttachment.loadOp = ToVulkanLoadOp( pInfo->pStencilAttachment->LoadOp );
			stencilAttachment.storeOp = ToVulkanStoreOp( pInfo->pStencilAttachment->StoreOp );

			if ( pInfo->pStencilAttachment->ResolveImage )
			{
				stencilAttachment.resolveImageView = *pInfo->pStencilAttachment->ResolveImage.As<CVulkanTexture2D>()->GetImageView();
				stencilAttachment.resolveImageLayout = ( VkImageLayout )pInfo->pStencilAttachment->ResolveImage->Layout;
				stencilAttachment.resolveMode = ToVulkanResolveMode( pInfo->pStencilAttachment->ResolveMode );
			}
		}
		
		VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
		renderingInfo.renderArea.extent = { pInfo->RenderArea.Width, pInfo->RenderArea.Height };
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.layerCount = 1;
        
		renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();

        vkCmdBeginRendering( cmd, &renderingInfo );

    };

    void CVulkanRenderer::EndRendering( u32 frameIndex ) NOEXCEPT
    {
        MW_PROFILE_FUNC;

        auto cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

        vkCmdEndRendering( cmd );

    }

	MW_NOTHROW void CVulkanRenderer::DispatchCompute( u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID /*= -1*/, DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		VkCommandBuffer hCmd = nullptr;

		if ( threadID < 0 )
			hCmd = *CVulkanRenderManager::GetCurrentRootGraphicsCommandBuffer();
		else
			hCmd = *CVulkanRenderManager::GetCurrentWorkerGraphicsCommandBuffer( threadID );

		if ( !hCmd )
			MW_ERROR( "Failed to get current worker or root command buffer. Discarding Dispatch" );	return;
		

		auto vkPipeline = hPipeline.As<CVulkanComputePipeline>();
        vkCmdBindDescriptorSets( hCmd, VK_PIPELINE_BIND_POINT_COMPUTE, *vkPipeline->GetVulkanPipelineSignature(), 0, descriptorCount, static_cast<VkDescriptorSet*>(*pDesciptors), 0, nullptr);

		vkCmdBindPipeline( hCmd, VK_PIPELINE_BIND_POINT_COMPUTE, *vkPipeline->GetVulkanPipeline() );

		vkCmdDispatch( hCmd, workgroup.x, workgroup.y, workgroup.z );
	}

	MW_NOTHROW void CVulkanRenderer::DispatchCompute2( u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID /*= -1 */ ) NOEXCEPT
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

	MW_NOTHROW void CVulkanRenderer::ProfileFrameData() NOEXCEPT
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

	MW_NOTHROW u32 CVulkanRenderer::AcquireNextImage( u32 frameIndex ) NOEXCEPT
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

	MW_NOTHROW void CVulkanRenderer::BindGraphicsPipeline( u32 frameIndex, Ref<IGraphicsPipeline> hPipeline, s32 MW_NULLABLE threadID /*= -1 */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding graphics pipeline bind on threadID {}.", threadID ); return;

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding graphics pipeline bind on threadID {}.", threadID, threadID ); return;

		auto vkPipeline = hPipeline.As<CVulkanGraphicsPipeline>();

		vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *vkPipeline->GetVulkanPipeline() );
	}

	MW_NOTHROW void CVulkanRenderer::BindDescriptors( 
		u32 frameIndex,
		PipelineSignature pSignature,
		DescriptorHandle* pDescriptors,
		size_t descriptorCount,
		u32 firstSet,
		s32 MW_NULLABLE threadID /*= -1 */ )
	{
		MW_PROFILE_FUNC;
		
		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding descriptor bind on threadID {}.", threadID ); return;

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding descriptor bind on threadID {}.", threadID, threadID ); return;

		// Bind descriptors for graphics
		vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ( VkPipelineLayout )pSignature, firstSet, descriptorCount, ( VkDescriptorSet* )pDescriptors, 0, nullptr );
		
		// Bind descriptors for compute
		vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ( VkPipelineLayout )pSignature, firstSet, descriptorCount, ( VkDescriptorSet* )pDescriptors, 0, nullptr );

	}

	MW_NOTHROW void CVulkanRenderer::SetDynamicViewports( u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		
		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic viewport state bind." ); return;

		if ( !pViewports )
			MW_API_ERROR( "pViewports is nullptr. Discarding dynamic viewport state bind." ); return;

		if ( !viewportCount )
			MW_API_ERROR( "viewportCount must be greater than 0. Discarding dynamic viewport state bind." ); return; 


		auto workerData = CVulkanRenderManager::GetWorkerFrameData();
		VkCommandBuffer rootCmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

		// Binds the viewports for the root command buffer.
		vkCmdSetViewport( rootCmd, ( u32 )firstViewport, ( u32 )viewportCount, ( VkViewport* )pViewports );
	
		// Binds the viewport for all worker command buffers since state isn't inherited from the primary command buffer. 
		for ( auto& worker : workerData )
			vkCmdSetViewport( worker.GraphicsCommandBuffers[frameIndex], ( u32 )firstViewport, ( u32 )viewportCount, ( VkViewport* )pViewports );
	}

	MW_NOTHROW void CVulkanRenderer::SetDynamicScissors( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind." ); return;

		if ( !pScissors )
			MW_API_ERROR( "pScissors is nullptr. Discarding dynamic scissor state bind." ); return;

		if ( !scissorCount )
			MW_API_ERROR( "scissorCount must be greater than 0. Discarding dynamic scissor state bind." ); return;

		std::vector<VkRect2D> scissors( scissorCount );

		auto i{ 0uz };
		for ( VkRect2D& scissor : scissors )
		{
			scissor.extent = { pScissors[i].Width, pScissors[i].Height };
			scissor.offset = {};
			i++;
		}
		auto workerData = CVulkanRenderManager::GetWorkerFrameData();
		VkCommandBuffer rootCmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

		vkCmdSetScissor( rootCmd, firstScissor, scissorCount, scissors.data() );
			
		for ( auto& worker : workerData )
			vkCmdSetScissor( worker.GraphicsCommandBuffers[frameIndex], firstScissor, scissorCount, scissors.data() );

	}


	NODISCARD static MW_NOTHROW VkCullModeFlagBits ToVulkanCullMode( ECullMode mode ) NOEXCEPT
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
	

	MW_NOTHROW void CVulkanRenderer::SetDynamicCullMode( u32 frameIndex, ECullMode cullMode ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind." ); return;
		
		auto workerData = CVulkanRenderManager::GetWorkerFrameData();
		VkCommandBuffer rootCmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );

		const auto cum = ToVulkanCullMode( cullMode );

		vkCmdSetCullMode( rootCmd, cum );

		for ( auto& worker : workerData )
			vkCmdSetCullMode( worker.GraphicsCommandBuffers[frameIndex], cum );

	}


	MW_NOTHROW void CVulkanRenderer::SetDynamicViewportsST( u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic viewport state bind on threadID {}.", threadID ); return;

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding dynamic viewport state bind on threadID {}.", threadID, threadID ); return;
		
		vkCmdSetViewport( cmd, firstViewport, viewportCount, ( VkViewport* )pViewports );
	}

	MW_NOTHROW void CVulkanRenderer::SetDynamicScissorsST( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		
		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind on threadID {}.", threadID ); return;

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding dynamic scissor state bind on threadID {}.", threadID, threadID ); return;

		std::vector<VkRect2D> scissors( scissorCount );
		auto i{ 0uz };
		for ( VkRect2D& scissor : scissors )
		{
			scissor.extent = { pScissors[i].Width, pScissors[i].Height };
			scissor.offset = {};
			i++;
		}

		vkCmdSetScissor( cmd, firstScissor, scissorCount, scissors.data() );

	}

	MW_NOTHROW void CVulkanRenderer::SetDynamicCullModeST( u32 frameIndex, ECullMode cullMode, s32 threadID ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( frameIndex > MFIF )
			MW_API_ERROR( "Pass invalid frameIndex: Greater than MaxFramesInFlight. Discarding dynamic scissor state bind on threadID {}.", threadID ); return;

		VkCommandBuffer cmd = nullptr;

		if ( threadID < 0 )
			cmd = *CVulkanRenderManager::GetRootGraphicsCommandBuffer( frameIndex );
		else
			cmd = *CVulkanRenderManager::GetWorkerCommandBuffer( threadID, frameIndex );

		if ( !cmd )
			MW_ERROR( "Failed to assign commandbuffer for thread id {}. Discarding dynamic scissor state bind on threadID {}.", threadID, threadID ); return;

		vkCmdSetCullMode( cmd, ToVulkanCullMode( cullMode ) );
	}

	MW_NOTHROW void CVulkanRenderer::BeginRootCommandbuffer( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::BeginRootGraphicsCommandBuffer( frameIndex );
	}

	void CVulkanRenderer::SubmitRootCommandbuffer( u32 frameIndex  )
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::EndWorkerGraphicsCommandBuffers( frameIndex );
		CVulkanRenderManager::SubmitRootGraphicsCommandBuffer( frameIndex );
	}

	MW_NOTHROW void CVulkanRenderer::BeginSecondaryCommandbuffers( u32 frameIndex  ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::BeginWorkerGraphicsCommandBuffers( frameIndex );
	}

	MW_NOTHROW void CVulkanRenderer::MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CVulkanRenderManager::EndWorkerGraphicsCommandBuffers( frameIndex );
	}


}