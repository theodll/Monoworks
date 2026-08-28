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
        Slang::ComPtr<slang::IGlobalSession>    CStaticRenderer::m_SlangGlobalSession;

		void CStaticRenderer::Init() noexcept
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

        };
        
        void CStaticRenderer::Shutdown() noexcept 
        {
            MW_PROFILE_FUNC;
            RHI::CPipelineManager::Shutdown();
            m_pInstance->Shutdown();
            MW_INFO( "Shutdown CStaticRenderer" );
        }; 

        MW_NOTHROW void CStaticRenderer::DispatchCompute( Ref<RHI::IComputePipeline> hPipeline, u32 workgroupX, u32 workgroupY, u32 workgroupZ, u32 MW_NULLABLE threadID ) NOEXCEPT
        {
            MW_PROFILE_FUNC;
            m_pInstance->DispatchCompute( hPipeline, workgroupX, workgroupY, workgroupZ, threadID );
        }

        void CStaticRenderer::BeginRendering() NOEXCEPT
        {
            MW_PROFILE_FUNC;
            m_pInstance->BeginRendering();
        };

        void CStaticRenderer::EndRendering() NOEXCEPT
        {
            MW_PROFILE_FUNC;

            m_pInstance->EndRendering();
            m_CurrentFrameIndex = ( m_CurrentFrameIndex + 1 ) % MFIF;
        };
}