#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/IndexBuffer.hh>
#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

#include <rhi/GraphicsAPI.hh>

#ifdef MW_ENABLE_MANUAL_RENDERDOC
#include <renderdoc_app.h>
#endif 

namespace Monoworks::RHI
{
    // For documentation refer to IGraphicsAPI
    class CVulkanRenderer final : public IGraphicsAPI
    {
        MW_NOTHROW void Init() NOEXCEPT override;
        MW_NOTHROW void Shutdown() NOEXCEPT override;
        
        // TODO: add push constants
        MW_NOTHROW void DispatchCompute(  u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT override;
        MW_NOTHROW void DispatchCompute2( u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT override;

        MW_NOTHROW void BeginRendering( const BeginRenderingInfo* pInfo ) NOEXCEPT override;
        MW_NOTHROW void EndRendering() NOEXCEPT override;

        MW_NOTHROW void ProfileFrameData() NOEXCEPT override;

        MW_NOTHROW u32 AcquireNextImage() NOEXCEPT override;

        MW_NOTHROW void BindGraphicsPipeline(   u32 frameIndex, Ref<IGraphicsPipeline> hPipeline, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT override;
        MW_NOTHROW void BindDescriptors(        u32 frameIndex, DescriptorSignature pSignature, DescriptorHandle* pDescriptors, size_t descriptorCount, u32 firstSet, s32 MW_NULLABLE threadID = -1 ) override;

        MW_NOTHROW void SetDynamicViewports( u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT override;
        MW_NOTHROW void SetDynamicScissors(   u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT override;
        MW_NOTHROW void SetDynamicCullMode(  u32 frameIndex, ECullMode cullMode ) NOEXCEPT override;

		MW_NOTHROW void SetDynamicViewportsST( u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID ) NOEXCEPT override;
		MW_NOTHROW void SetDynamicScissorsST( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID ) NOEXCEPT override;
		MW_NOTHROW void SetDynamicCullModeST( u32 frameIndex, ECullMode cullMode, s32 threadID ) NOEXCEPT override;


        MW_NOTHROW void BeginRootCommandbuffer( u32 frameIndex  ) NOEXCEPT override;
        void SubmitRootCommandbuffer( u32 frameIndex ) override;

        MW_NOTHROW void BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT override;
        MW_NOTHROW void MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT override;

    private:

#ifdef MW_ENABLE_MANUAL_RENDERDOC
		RENDERDOC_API_1_1_2* m_RenderDocAPI = nullptr;
#endif 

        Ref<IVertexBuffer> m_Vertices;
        Ref<IIndexBuffer> m_Indices;
        Ref<IGraphicsPipeline> m_Pipeline;
    };
}