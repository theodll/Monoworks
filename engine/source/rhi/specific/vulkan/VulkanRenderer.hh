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
         void MWAPI_CALL Init() NOEXCEPT override;
         void MWAPI_CALL Shutdown() NOEXCEPT override;
        
         void MWAPI_CALL WaitRenderingIdle() NOEXCEPT override;

        // TODO: add push constants
         void MWAPI_CALL DispatchCompute(
             u32 frameIndex, 
             Ref<IComputePipeline> hPipeline,
             Vector workgroup,
             std::span<DescriptorHandle> ppDesciptors, 
             s32 threadID = -1 ) NOEXCEPT override;

         void MWAPI_CALL DispatchCompute2(
             u32 frameIndex,
             Ref<IComputePipeline> hPipeline,
             Vector workgroup,
             s32 MW_NULLABLE threadID = -1 ) NOEXCEPT override;

         void MWAPI_CALL BeginRendering(
             u32 frameIndex,
             const BeginRenderingInfo* pInfo ) NOEXCEPT override;

         void MWAPI_CALL EndRendering( u32 frameIndex ) NOEXCEPT override;

         void MWAPI_CALL ProfileFrameData() NOEXCEPT override;

         u32  MWAPI_CALL AcquireNextImage( u32 frameIndex ) NOEXCEPT override;
         void MWAPI_CALL Present( MAYBE_UNUSED u32 frameIndex ) NOEXCEPT override;

         void MWAPI_CALL BindGraphicsPipeline(
             u32 frameIndex, 
             Ref<IGraphicsPipeline> hPipeline, 
             s32 threadID = -1 ) NOEXCEPT override;

         void MWAPI_CALL BindDescriptors(
             u32 frameIndex,
             PipelineSignature pSignature,
             std::span<DescriptorHandle> ppDescriptors,
             u32 firstSet,
             s32 threadID = -1,
             RHI::EPipelineBindPoint bindpoint = RHI::MW_PIPELINE_BIND_POINT_BOTH ) override;

         void MWAPI_CALL DrawStaticMeshIndexed(
             u32 frameIndex,
             Ref<CMesh> hMesh,
             const Matrix& transform,
             s32 threadID = -1,
             Ref<IGraphicsPipeline> hPipelineOverride = nullptr ) NOEXCEPT override;

         void MWAPI_CALL SetDynamicViewports(
             u32 frameIndex,
             std::span<const Viewport> ppViewports,
             size_t firstViewport ) NOEXCEPT override;

         void MWAPI_CALL SetDynamicScissors(
             u32 frameIndex,
             std::span<const SExtent2D> ppScissors,
             size_t firstScissor ) NOEXCEPT override;

         void MWAPI_CALL SetDynamicCullMode(
             u32 frameIndex, 
             ECullMode cullMode ) NOEXCEPT override;

		 void MWAPI_CALL SetDynamicViewportsST(
             u32 frameIndex,
             std::span<const Viewport> ppViewports, 
             size_t firstViewport, 
             s32 threadID ) NOEXCEPT override;

		 void MWAPI_CALL SetDynamicScissorsST(
             u32 frameIndex,
             std::span<const SExtent2D> ppScissors,
             size_t firstScissor,
             s32 threadID ) NOEXCEPT override;

		 void MWAPI_CALL SetDynamicCullModeST(
             u32 frameIndex,
             ECullMode cullMode,
             s32 threadID ) NOEXCEPT override;


         void MWAPI_CALL BeginRootCommandbuffer( u32 frameIndex  ) NOEXCEPT override;
         void MWAPI_CALL SubmitRootCommandbuffer( u32 frameIndex ) override;

         void MWAPI_CALL BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT override;
         void MWAPI_CALL MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT override;

    private:

#ifdef MW_ENABLE_MANUAL_RENDERDOC
		RENDERDOC_API_1_1_2* m_RenderDocAPI = nullptr;
#endif 

    };
}