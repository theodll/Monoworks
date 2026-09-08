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

        virtual MW_NOTHROW void ProfileFrameData() NOEXCEPT = 0;

        /**
         * @brief Begins the root command buffer.
         * Does not begin the secondary command buffers.
         * Unlike BeginSecondaryCommandbuffers, this command may only be called once in the entire render loop.
         * This command must not be submitted inside any job or asynchronous action.
         */
        virtual MW_NOTHROW void BeginRootCommandbuffer( u32 frameIndex  ) NOEXCEPT = 0;

        /**
         * @brief Ends and submits the root command buffer.
         * @throw Will throw CGPUFatalExeption if command buffer submission failed fatally (MW_ERROR_GPU_DEVICE_LOST). 
         * All secondaries must be merged manually via IGraphicsAPI::MergeSecondaryCommandbuffers before ending and submitting the root commandbuffer.
         * If this condition is not fulfilled this may lead to undefined behavior and will result in API errors.
         */ 
        virtual void SubmitRootCommandbuffer( u32 frameIndex  ) = 0;

        /**
         * @brief Resets and Begins all secondary command buffers.
         * This must not be submitted inside any job or any kind if asynchronous action 
         */
        virtual MW_NOTHROW void BeginSecondaryCommandbuffers( u32 frameIndex  ) NOEXCEPT = 0;
        /**
         * @brief Merges the commands of all secondary / worker command buffers into the root command buffer.
         * This batches command buffer execution and must not be submitted inside any job or any kind of asynchronous action.
         */
        virtual MW_NOTHROW void MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT = 0;

        /**
         * @brief Begins a rendering scope (vkCmdBeginRendering) based on the BeginRenderingInfo given.
		 * Rendering state commands are primary command buffer commands.
		 * These commands will be recorded into the primary command buffer and must not be submitted inside any job or any kind of asynchronous action.
		 * Furthermore, all commands recorded into secondary command buffers during this scope must be merged into the primary command buffer before EndRendering.
         */
        virtual MW_NOTHROW void BeginRendering( const BeginRenderingInfo* pInfo ) NOEXCEPT = 0; 

        /**
		* @brief Ends a rendering scope (vkCmdEndRendering).
		* Rendering state commands are primary command buffer commands.
		* These commands will be recorded into the primary command buffer and must not be submitted inside any job or any kind of asynchronous action.
		* Furthermore, all commands recorded into secondary command buffers during this scope must be merged into the primary command buffer before EndRendering.
		*/
        virtual MW_NOTHROW void EndRendering() NOEXCEPT = 0;

        /**
         * @brief Acquires the next image from the selected presenter.
         */
        virtual MW_NOTHROW u32  AcquireNextImage() NOEXCEPT = 0;

        /**
         * @brief Binds the graphics pipeline (hPipeline) to the selected command buffer based on threadID.
         * The default value for threadID (-1) is the root command buffer corresponding to the main thread.
         */
        virtual MW_NOTHROW void BindGraphicsPipeline(   u32 frameIndex, Ref<IGraphicsPipeline> hPipeline, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT = 0;
        
        /**
         * @brief Binds all descriptors in the pDescriptors array to the selected command buffer based on threadID for all subsequent graphics and compute pipelines.
         * The default value for threadID (-1) is the root command buffer corresponding to the main thread.
         */
        virtual MW_NOTHROW void BindDescriptors(        u32 frameIndex, DescriptorSignature pSignature, DescriptorHandle* pDescriptors, size_t descriptorCount, u32 firstSet, s32 MW_NULLABLE threadID = -1 ) = 0;
		
        
        /// @brief Binds all the viewports in the pViewports array as dynamic state to the root command buffer and all worker command buffers. 
		virtual MW_NOTHROW void SetDynamicViewports( u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT = 0;
        /// @brief Binds all the scissors in the pScissors array as dynamic state to the root command buffer and all worker command buffers.
        virtual MW_NOTHROW void SetDynamicScissors( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT = 0;
		/// @brief Binds the cull mode as dynamic state to the root command buffer and all worker command buffers.
		virtual MW_NOTHROW void SetDynamicCullMode( u32 frameIndex, ECullMode cullMode ) NOEXCEPT = 0;

        /**
         * @brief Binds all the viewports in the pViewports array as dynamic state to the selected command buffer based on threadID.
         * The default value for threadID (-1) is the root command buffer corresponding to the main thread.
         */
        virtual MW_NOTHROW void SetDynamicViewportsST( u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID ) NOEXCEPT = 0;
        
        /**
        * @brief Binds all the scissors in the pScissors array as dynamic state to the selected command buffer based on threadID.
        * The default value for threadID (-1) is the root command buffer corresponding to the main thread.
        */
        virtual MW_NOTHROW void SetDynamicScissorsST( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID ) NOEXCEPT = 0;

        /**
        * @brief Binds the cullMode dynamic state to the selected command buffer based on threadID.
        * The default value for threadID (-1) is the root command buffer corresponding to the main thread.
        */
        virtual MW_NOTHROW void SetDynamicCullModeST( u32 frameIndex, ECullMode cullMode, s32 threadID ) NOEXCEPT = 0;

        /**
         * @brief Dispatches the compute pipeline (hPipeline) with descriptors.
         */
        virtual MW_NOTHROW void DispatchCompute( u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT = 0;
        
         /**
         * @brief Dispatches the compute pipeline (hPipeline).
         */
        virtual MW_NOTHROW void DispatchCompute2( u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT = 0;

    };

}
