#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

namespace Monoworks::RHI 
{
    enum ERenderingFlagBits
    {
        MW_RENDERING_FLAGS_NONE = 0x0,
        MW_RENDERING_FLAGS_WITH_SECONDARY_COMMAND_BUFFERS_BIT = 0x1b,

        MW_RENDERING_FLAGS_MAX_ENUM = 0x7FFFFFFF,
    };
    using ERenderingFlags = flags_t;

    enum EResolveModeBits
    {
        MW_RESOLVE_MODE_NONE = 0x0,
        MW_RESOLVE_MODE_SAMPLE_ZERO_BIT = 0b1,
        MW_RESOLVE_MODE_AVERAGE_BIT = 0b10,
        MW_RESOLVE_MODE_MINIMUM_BIT = 0b100,
        MW_RESOLVE_MODE_MAXIMUM_BIT = 0b1000 
    };
    using EResolveMode = flags_t; 

    enum EAttachmentLoadOp
    {
        MW_ATTACHMENT_LOAD_OP_LOAD = 0,
        MW_ATTACHMENT_LOAD_OP_CLEAR = 1,
        MW_ATTACHMENT_LOAD_OP_DONT_CARE = 2
    };

    enum EAttachmentStoreOp
    {
        MW_ATTACHMENT_STORE_OP_STORE = 0,
        MW_ATTACHMENT_STORE_OP_DONT_CARE = 1
    };

    struct RenderingAttachmentInfo 
    {  
        Ref<ITexture2D> AttachmentImage;
     
        // Used for multisampling.
        EResolveMode  MW_NULLABLE ResolveMode = MW_RESOLVE_MODE_NONE;
        Ref<ITexture2D> MW_NULLABLE ResolveImage = nullptr; 
        
        EAttachmentLoadOp  LoadOp = MW_ATTACHMENT_LOAD_OP_CLEAR;
        EAttachmentStoreOp StoreOp = MW_ATTACHMENT_STORE_OP_STORE;
    };

    struct BeginRenderingInfo 
    {
        /// @brief Bitfield of ERenderingFlagBits.  
        ERenderingFlags Flags;
        /// @brief The render area that is affected by the render pass instance.
        SExtent2D RenderArea;
        /// @brief Number of color attachments inside the pColorAttachments array.
        u32 ColorAttachmentCount;
        /// @brief C-Style array of color attachments.
        const RenderingAttachmentInfo* pColorAttachments;
        /// @brief Optional depth attachment
        const RenderingAttachmentInfo* MW_NULLABLE pDepthAttachment = nullptr;
        /// @brief Optional stencil attachment
        const RenderingAttachmentInfo* MW_NULLABLE pStencilAttachment = nullptr;
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
        virtual MW_NOTHROW void BeginRendering( const BeginRenderingInfo* pInfo ) NOEXCEPT = 0; 
        virtual MW_NOTHROW void EndRendering() NOEXCEPT = 0;

        virtual MW_NOTHROW void DispatchCompute( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT;
        virtual MW_NOTHROW void DispatchCompute2( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT;

    };

}
