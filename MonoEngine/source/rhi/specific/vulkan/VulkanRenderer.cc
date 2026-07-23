#include <common/Base.hh>

#include "VulkanRenderer.hh"

namespace Monoworks::RHI 
{
    void CVulkanRenderer::Init() noexcept override
    {
        MW_PROFILE_FUNC;
        MW_INFO("Initialize CVulkanRenderer");
    } 

    void CVulkanRenderer::Shutdown() noexcept override 
    {
        MW_PROFILE_FUNC;
        MW_INFO("Shutdown CVulkanRenderer");
    }
}