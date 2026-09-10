#include <mwpch.hh>
#include <core/Application.hh>
#include <events/EventManager.hh>

#include <rhi/specific/vulkan/VulkanRenderer.hh>

#include "StaticRenderer.hh"

namespace Monoworks 
{
        Ref<RHI::IGraphicsAPI> CStaticRenderer::m_pInstance;
        u32                    CStaticRenderer::m_CurrentFrameIndex;
		u32                    CStaticRenderer::m_CurrentImageIndex;

		Monoworks::SExtent2D CStaticRenderer::m_RenderableExtent;

		Slang::ComPtr<slang::IGlobalSession>    CStaticRenderer::m_SlangGlobalSession;

        
		MW_NOTHROW void CStaticRenderer::Init() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			MW_INFO( "Initialize CStaticRenderer" );
			switch ( CApplication::GetGraphicsAPI() )
			{
			case MW_GAPI_NONE: MW_ASSERT( false, "Headless mode not supported" ); break;
			case MW_GAPI_VULKAN: m_pInstance = Ref<RHI::CVulkanRenderer>::Create(); break;
			}

			m_pInstance->Init();

			slang::createGlobalSession( m_SlangGlobalSession.writeRef() );

			RHI::CPipelineManager::Init();

			CEventManager::Subscribe( MW_EVENT_APP_FRAME, +[]( SEvent& e )
				{
					MW_PROFILE_FUNC;
					if ( RHI::CPipelineManager::GetTotalCompiledPipelineCount() < RHI::CPipelineManager::GetTotalPipelineCount() )
					{
						RHI::CPipelineManager::BatchCompile();
					}
					return false;
				} );
		}

		MW_NOTHROW void CStaticRenderer::Shutdown() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			RHI::CPipelineManager::Shutdown();
			m_pInstance->Shutdown();
			MW_INFO( "Shutdown CStaticRenderer" );
		}

		MW_NOTHROW void CStaticRenderer::ProfileFrameData() NOEXCEPT
		{
			MW_PROFILE_FUNC;
		}

		MW_NOTHROW void CStaticRenderer::BeginRootCommandbuffer( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BeginRootCommandbuffer( frameIndex );
		}

		void CStaticRenderer::SubmitRootCommandbuffer( u32 frameIndex )
		{
			MW_PROFILE_FUNC;
			m_pInstance->SubmitRootCommandbuffer( frameIndex );
			m_CurrentFrameIndex = ( m_CurrentFrameIndex + 1 ) % MFIF;
		}

		MW_NOTHROW void CStaticRenderer::BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BeginSecondaryCommandbuffers( frameIndex );
		}

		MW_NOTHROW void CStaticRenderer::MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->MergeSecondaryCommandbuffers( frameIndex );
		}

		MW_NOTHROW void CStaticRenderer::DispatchCompute( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID /*= -1*/, RHI::DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->DispatchCompute( frameIndex, hPipeline, workgroup, threadID, pDesciptors, descriptorCount );
		}

		MW_NOTHROW void CStaticRenderer::DispatchCompute2( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID /*= -1 */ ) NOEXCEPT
		{
            MW_PROFILE_FUNC;
            return m_pInstance->DispatchCompute2( frameIndex, hPipeline, workgroup, threadID );
		}

		void CStaticRenderer::BeginRendering( u32 frameIndex, const RHI::BeginRenderingInfo* pInfo ) NOEXCEPT
        {
            MW_PROFILE_FUNC;
            return m_pInstance->BeginRendering( frameIndex, pInfo );
        };

        void CStaticRenderer::EndRendering( u32 frameIndex ) NOEXCEPT
        {
            MW_PROFILE_FUNC;
            return m_pInstance->EndRendering( frameIndex );
        };


		MW_NOTHROW u32 CStaticRenderer::AcquireNextImage( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->AcquireNextImage( frameIndex );
		}

		MW_NOTHROW void CStaticRenderer::BindGraphicsPipeline( u32 frameIndex, Ref<RHI::IGraphicsPipeline> hPipeline, s32 MW_NULLABLE threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BindGraphicsPipeline( frameIndex, hPipeline, threadID );
		}

		MW_NOTHROW void CStaticRenderer::BindDescriptors( u32 frameIndex, RHI::DescriptorSignature pSignature, RHI::DescriptorHandle* pDescriptors, size_t descriptorCount, u32 firstSet, s32 MW_NULLABLE threadID /*= -1 */ )
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BindDescriptors( frameIndex, pSignature, pDescriptors, descriptorCount, firstSet, threadID );
		}

		MW_NOTHROW void CStaticRenderer::SetDynamicViewports( u32 frameIndex, const RHI::Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicViewports( frameIndex, pViewports, viewportCount, firstViewport );
		}

		MW_NOTHROW void CStaticRenderer::SetDynamicScissors( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicScissors( frameIndex, pScissors, scissorCount, firstScissor );
		}

		MW_NOTHROW void CStaticRenderer::SetDynamicCullMode( u32 frameIndex, RHI::ECullMode cullMode ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicCullMode( frameIndex, cullMode );
		}

		MW_NOTHROW void CStaticRenderer::SetDynamicViewportsST( u32 frameIndex, const RHI::Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicViewportsST( frameIndex, pViewports, viewportCount, firstViewport, threadID );
		}

		MW_NOTHROW void CStaticRenderer::SetDynamicScissorsST( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicScissorsST( frameIndex, pScissors, scissorCount, firstScissor, threadID );
		}

		MW_NOTHROW void CStaticRenderer::SetDynamicCullModeST( u32 frameIndex, RHI::ECullMode cullMode, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicCullModeST( frameIndex, cullMode, threadID );
		}

}