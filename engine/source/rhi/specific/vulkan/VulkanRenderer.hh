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
         void Init() NOEXCEPT override;
         void Shutdown() NOEXCEPT override;
        
         void WaitRenderingIdle() NOEXCEPT override;

        // TODO: add push constants
         void DispatchCompute(        u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, DescriptorHandle* pDesciptors, size_t descriptorCount, s32 threadID = -1 ) NOEXCEPT override;
         void DispatchCompute2(       u32 frameIndex, Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT override;

         void BeginRendering(         u32 frameIndex, const BeginRenderingInfo* pInfo ) NOEXCEPT override;
         void EndRendering(           u32 frameIndex ) NOEXCEPT override;

         void ProfileFrameData() NOEXCEPT override;

         u32 AcquireNextImage( u32 frameIndex ) NOEXCEPT override;
         void Present( MAYBE_UNUSED u32 frameIndex ) NOEXCEPT override;

         void BindGraphicsPipeline(   u32 frameIndex, Ref<IGraphicsPipeline> hPipeline, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT override;
         void BindDescriptors(        u32 frameIndex, PipelineSignature pSignature, DescriptorHandle* pDescriptors, size_t descriptorCount, u32 firstSet, s32 MW_NULLABLE threadID = -1, RHI::EPipelineBindPoint bindpoint = RHI::MW_PIPELINE_BIND_POINT_BOTH ) override;
         void DrawStaticMeshIndexed(  u32 frameIndex, Ref<CMesh> hMesh, const Matrix& transform, s32 threadID = -1, Ref<IGraphicsPipeline> hPipelineOverride = nullptr ) NOEXCEPT override;

         void SetDynamicViewports(    u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT override;
         void SetDynamicScissors(     u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT override;
         void SetDynamicCullMode(     u32 frameIndex, ECullMode cullMode ) NOEXCEPT override;

		 void SetDynamicViewportsST(  u32 frameIndex, const Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID ) NOEXCEPT override;
		 void SetDynamicScissorsST(   u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID ) NOEXCEPT override;
		 void SetDynamicCullModeST(   u32 frameIndex, ECullMode cullMode, s32 threadID ) NOEXCEPT override;


         void BeginRootCommandbuffer( u32 frameIndex  ) NOEXCEPT override;
        void SubmitRootCommandbuffer(           u32 frameIndex ) override;

         void BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT override;
         void MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT override;

    private:

#ifdef MW_ENABLE_MANUAL_RENDERDOC
		RENDERDOC_API_1_1_2* m_RenderDocAPI = nullptr;
#endif 

    };
}