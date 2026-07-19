#pragma once
#include <common/Base.hh>

#include <rhi/GraphicsAPI.hh>

namespace Monoworks::RHI
{
    class CVulkanRenderer : public IGraphicsAPI  
    {
        virtual void Init() override noexcept;
        virtual void Shutdown() override noexcept; 
    }
}