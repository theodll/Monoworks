#pragma once
#include <common/Base.hh>

namespace Monoworks::RHI 
{
    
    class IGraphicsAPI 
    {
    public:
        virtual ~IGraphicsAPI() = default;

        virtual void Init() noexcept = 0;
        virtual void Shutdown() noexcept = 0;
    };

}
