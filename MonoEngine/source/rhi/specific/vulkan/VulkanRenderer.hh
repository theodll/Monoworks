#pragma once
#include <common/Base.hh>

#include <rhi/GraphicsAPI.hh>

namespace Monoworks::RHI
{
    class CVulkanRenderer : public IGraphicsAPI
    {
        virtual void Init() NOEXCEPT override;
        virtual void Shutdown() NOEXCEPT override;
    };
}