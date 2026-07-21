#include <mwpch.hh>
#include <core/Application.hh>

#include "StaticRenderer.hh"

#include <rhi/specific/vulkan/VulkanRenderer.hh>

namespace Monoworks 
{
        Ref<RHI::IGraphicsAPI> CStaticRenderer::m_Instance;

        void CStaticRenderer::Init() noexcept
        {
            MW_PROFILE_FUNC;

            switch (CApplication::GetGraphicsAPI())
            {
            case MW_GAPI_NONE: MW_ASSERT("Headless mode not supported");
            case MW_GAPI_VULKAN: m_Instance = Ref<RHI::CVulkanRenderer>::Create(); break;
            }

            m_Instance->Init();

        };
        
        void CStaticRenderer::Shutdown() noexcept 
        {
            MW_PROFILE_FUNC;
            m_Instance->Shutdown();
        }; 
}