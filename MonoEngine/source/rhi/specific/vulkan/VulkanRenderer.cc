#include <mwpch.hh>

#include <fstream>

#include <rhi/specific/vulkan/VulkanRenderManager.hh>
#include <rhi/specific/vulkan/VulkanContext.hh>
#include <rhi/specific/vulkan/VulkanPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>
#include <rhi/specific/vulkan/VulkanGraphicsPipeline.hh>
#include <rhi/specific/vulkan/VulkanVertexBuffer.hh>
#include <rhi/specific/vulkan/VulkanIndexBuffer.hh>

#include <rhi/agnostic/IndexBuffer.hh>

#include <renderer/StaticRenderer.hh>
#include <core/Application.hh>

#include "VulkanRenderer.hh"

namespace Monoworks::RHI 
{
	static std::vector<char> readFile( const std::string& filename, size_t* size ) {
		std::ifstream file( filename, std::ios::ate | std::ios::binary );

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
        MW_INFO("Initialize CVulkanRenderer");
        CVulkanRenderManager::Init();

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

		SPipelineCreationInfo pipelineInfo{};

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
		m_Vertices = IVertexBuffer::Create( quadVertices.data(), quadVertices.size(), sizeof( SVertex ), true );

		std::vector<Index> indices = { 0, 1, 2, 2, 3, 0 };
        m_Indices = IIndexBuffer::Create( indices.data(), indices.size(), 0, true );
    } 

    void CVulkanRenderer::Shutdown() NOEXCEPT 
    {
        MW_PROFILE_FUNC;
        CVulkanRenderManager::Shutdown();
        MW_INFO( "Shutdown CVulkanRenderer" );
    }

    void CVulkanRenderer::BeginRendering() NOEXCEPT
    {
        MW_PROFILE_FUNC;
        u32* imageIndex = CStaticRenderer::GetCurrentImageIndexPtr();
        u32 frameIndex = CStaticRenderer::GetCurrentFrameIndex();

        auto presenter = CVulkanContext::GetPresenter();

        if ( CApplication::GetCreateInfos()->UseSDL && CApplication::GetCreateInfos()->UseSwapchain )
        {
            SVulkanSDLPresentationAcquisitionInfo acquisitionInfo{};
            acquisitionInfo.pDevice = CVulkanContext::GetDevice()->GetDevice();
            acquisitionInfo.pPhysDevice = CVulkanContext::GetDevice()->GetPhysicalDevice();
            acquisitionInfo.pVulkanDevice = CVulkanContext::GetDevice();
            acquisitionInfo.pImageAvailableSemaphore = CVulkanRenderManager::GetImageAvailableSemaphore( frameIndex );
            acquisitionInfo.pInFlightFence = CVulkanRenderManager::GetInFlightFence( frameIndex );

            *imageIndex = presenter->Acquire( &acquisitionInfo );
        }
        else if ( CApplication::GetCreateInfos()->UseQt )
        {
            SVulkanQtPresentationAcquisitionInfo acquisitionInfo{};
            acquisitionInfo.pGraphicsQueue = CVulkanContext::GetDevice()->GetGraphicsQueue();
            acquisitionInfo.pImageAvailableSemaphore = CVulkanRenderManager::GetImageAvailableSemaphore( frameIndex );
            acquisitionInfo.pInFlightFence = CVulkanRenderManager::GetInFlightFence( frameIndex );
            acquisitionInfo.pVulkanDevice = CVulkanContext::GetDevice();

            *imageIndex = presenter->Acquire( &acquisitionInfo );
        }

        CVulkanRenderManager::BeginRootCommandBuffer( frameIndex );
        CVulkanRenderManager::BeginWorkerCommandBuffers( frameIndex );


        auto cmd = *CVulkanRenderManager::GetCurrentRootCommandBuffer();
        auto width = CApplication::GetCreateInfos()->RenderableExtent.Width;
        auto height = CApplication::GetCreateInfos()->RenderableExtent.Height;

        auto siye = presenter->GetSwapchainImages().size();
        MW_ASSERT( *imageIndex < presenter->GetSwapchainImages().size(), "Invalid swapchain image index" );

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = *presenter->GetSwapchainImages()[*imageIndex].As<CVulkanTexture2D>()->GetImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { {{ 0.0f, 1.0f, 1.0f, 1.0f }} };

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.extent = { width, height };
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        vkCmdBeginRendering( cmd, &renderingInfo );


        // TODO: put this somewhere else
        VkRect2D scissor{};
        scissor.extent = { width, height };
        scissor.offset = { 0, 0 };
        vkCmdSetScissor( cmd, 0, 1, &scissor );

        VkViewport vulkanViewport{};
        vulkanViewport.height = ( float )height;
        vulkanViewport.width = ( float )width;
        vulkanViewport.x = 0;
        vulkanViewport.y = 0;
        vulkanViewport.maxDepth = 1.0f;
        vulkanViewport.minDepth = 0.0f;
        vkCmdSetViewport( cmd, 0, 1, &vulkanViewport );

        vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_Pipeline.As<CVulkanGraphicsPipeline>()->GetVulkanPipeline() );

        u64 offset[] = { 0 }; 
        vkCmdBindVertexBuffers( cmd, 0, 1, m_Vertices.As<CVulkanVertexBuffer>()->GetVulkanBuffer(), offset );
        vkCmdBindIndexBuffer( cmd, *m_Indices.As<CVulkanIndexBuffer>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32 );

        vkCmdDrawIndexed( cmd, m_Indices->GetCount(), 1, 0, 0, 0 );

    };

    void CVulkanRenderer::EndRendering() NOEXCEPT
    {
        MW_PROFILE_FUNC;
        u32 frameIndex = CStaticRenderer::GetCurrentFrameIndex();
        auto presenter = CVulkanContext::GetPresenter();

        auto cmd = *CVulkanRenderManager::GetCurrentRootCommandBuffer();

        vkCmdEndRendering( cmd );

        CVulkanRenderManager::EndWorkerCommandBuffers( frameIndex );

        if ( CApplication::GetCreateInfos()->UseSDL && CApplication::GetCreateInfos()->UseSwapchain )
        {
            const auto imageIndex = CStaticRenderer::GetCurrentImageIndex();
            auto& swapchainImages = presenter->GetSwapchainImages();
            MW_ASSERT( imageIndex < swapchainImages.size(), "Invalid swapchain image index" );

            auto& swapchainImage = swapchainImages[imageIndex];
            if ( swapchainImage->Layout != MW_IMAGE_LAYOUT_PRESENT_SRC_KHR )
            {
                auto sourceStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                if ( swapchainImage->Layout == MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
                {
                    sourceStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                else if ( swapchainImage->Layout != MW_IMAGE_LAYOUT_UNDEFINED )
                {
                    sourceStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                }

                auto vulkanTexture = swapchainImage.As<CVulkanTexture2D>();
                TransitionImageLayout2(
                    *CVulkanRenderManager::GetRootCommandBuffer( frameIndex ),
                    *vulkanTexture->GetImage(),
                    ( VkImageLayout )swapchainImage->Layout,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    sourceStageMask,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );

                swapchainImage->Layout = MW_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                swapchainImage->PipelineFlags = MW_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }
        }

        CVulkanRenderManager::EndRootCommandBuffer( frameIndex );
        CVulkanRenderManager::SubmitRootCommandBuffer( frameIndex );

        if ( CApplication::GetCreateInfos()->UseSDL && CApplication::GetCreateInfos()->UseSwapchain )
        {
            CVulkanContext::GetUploader()->Begin();
            SVulkanSDLPresentationTransitionPresentInfo renderInfo{};
            renderInfo.pCmdBuffer = CVulkanContext::GetUploader()->GetCommandBuffer();
            renderInfo.ImageIndex = CStaticRenderer::GetCurrentImageIndex();

            presenter->TransitionPresent( &renderInfo );
            CVulkanContext::GetUploader()->End();

            SVulkanSDLPresentationPresentInfo presentInfo{};
            presentInfo.pDevice = CVulkanContext::GetDevice()->GetDevice();
            presentInfo.pPhysDevice = CVulkanContext::GetDevice()->GetPhysicalDevice();
            presentInfo.pImageIndex = CStaticRenderer::GetCurrentImageIndexPtr();
            presentInfo.pPresentQueue = CVulkanContext::GetDevice()->GetPresentQueue();
            presentInfo.pRenderFinishedSemaphore = CVulkanRenderManager::GetRenderFinishedSemaphore( frameIndex );
            presentInfo.pVulkanDevice = CVulkanContext::GetDevice();

            presenter->Present( &presentInfo );
        }
        
    };
}