#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/DescriptorManager.h>

namespace Monoworks::RHI 
{
    class IGraphicsAPI 
    {
    public:
        virtual ~IGraphicsAPI() = default;

        virtual MW_NOTHROW void Init() NOEXCEPT = 0;
        virtual MW_NOTHROW void Shutdown() NOEXCEPT = 0;

        virtual MW_NOTHROW void DispatchCompute( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, DescriptorHandle* pDesciptors, size_t pDescriptorCount ) NOEXCEPT;
        virtual MW_NOTHROW void DispatchCompute( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT;

        virtual void BeginRendering() NOEXCEPT = 0;
        virtual void EndRendering() NOEXCEPT = 0;
    };

}
