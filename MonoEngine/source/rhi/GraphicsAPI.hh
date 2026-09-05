#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

namespace Monoworks::RHI 
{
    struct RenderingAttachmentInfo 
    {
        

    };

    struct BeginRenderingInfo 
    {
        /// @brief The render area that is affected by the render pass instance.
        SExtent2D RenderArea;
        /// @brief Number of color attachments inside the pColorAttachments array.
        u32 ColorAttachmentCount;
        /// @brief C-Style array of color attachments.
        const RenderingAttachmentInfo* pColorAttachments;
        /// @brief Optional depth attachment
        const RenderingAttachmentInfo* MW_NULLABLE pDepthAttachment;
        /// @brief Optional stencil attachment
        const RenderingAttachmentInfo* MW_NULLABLE pStencilAttachment;
    };

    class IGraphicsAPI 
    {
    public:
        virtual ~IGraphicsAPI() = default;

        virtual MW_NOTHROW void Init() NOEXCEPT = 0;
        virtual MW_NOTHROW void Shutdown() NOEXCEPT = 0;

        // NOTE: Rendering state commands are primary commandbuffer commands. 
        // These commands will be recorded into the primary command buffer and must not be submitted inside any job or any kind of asynchronous action.
        // Furthermore, all commands recorded into secondary command buffers during this scope must be merged into the primary command buffer vi 
        virtual MW_NOTHROW void BeginRendering(  );
        virtual MW_NOTHROW void EndRendering();

        virtual MW_NOTHROW void DispatchCompute( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT;
        virtual MW_NOTHROW void DispatchCompute2( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT;

    };

}
