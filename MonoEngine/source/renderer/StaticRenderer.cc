#include <mwpch.hh>
#include <core/Application.hh>

#include <rhi/specific/vulkan/VulkanRenderer.hh>

#include "StaticRenderer.hh"

namespace Monoworks 
{
        Ref<RHI::IGraphicsAPI> CStaticRenderer::m_pInstance;
        u32                    CStaticRenderer::m_CurrentFrameIndex;
		u32                    CStaticRenderer::m_CurrentImageIndex;
         
		void CStaticRenderer::Init() noexcept
        {
            MW_PROFILE_FUNC;
            MW_INFO( "Initialize CStaticRenderer" );
            switch ( CApplication::GetGraphicsAPI() )
            {
            case MW_GAPI_NONE: MW_ASSERT( "Headless mode not supported" ); break;
            case MW_GAPI_VULKAN: m_pInstance = Ref<RHI::CVulkanRenderer>::Create(); break;
            }

            m_pInstance->Init();

        };
        
        void CStaticRenderer::Shutdown() noexcept 
        {
            MW_PROFILE_FUNC;
            m_pInstance->Shutdown();
            MW_INFO( "Shutdown CStaticRenderer" );
        }; 


        void CStaticRenderer::BeginRendering() NOEXCEPT
        {
            MW_PROFILE_FUNC;
            m_pInstance->BeginRendering();
        };

        void CStaticRenderer::EndRendering() NOEXCEPT
        {
            MW_PROFILE_FUNC;

            m_pInstance->EndRendering();
            u32 temp = m_CurrentFrameIndex;
            m_CurrentFrameIndex = ( m_CurrentFrameIndex + 1 ) % MFIF;
        };
}