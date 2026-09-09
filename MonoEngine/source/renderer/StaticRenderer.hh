#pragma once
#include <common/Base.hh>
#include <common/Memory.hh>
#include <common/Math.hh>

#include <rhi/agnostic/PipelineManager.hh>
#include <rhi/GraphicsAPI.hh>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Monoworks 
{
	
    // For Documentation of undocumented parts, refer to IGraphicsAPI.
    class CStaticRenderer
    {
    public:
        static MW_NOTHROW void Init() NOEXCEPT;
        static MW_NOTHROW void Shutdown() NOEXCEPT;

        static MW_NOTHROW void ProfileFrameData() NOEXCEPT;

        static MW_NOTHROW void BeginRootCommandbuffer( u32 frameIndex ) NOEXCEPT;
        static void SubmitRootCommandbuffer( u32 frameIndex );

		static MW_NOTHROW void BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT;
		static MW_NOTHROW void MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT;

		static MW_NOTHROW void BeginRendering( u32 frameIndex, const RHI::BeginRenderingInfo* pInfo ) NOEXCEPT;
		static MW_NOTHROW void EndRendering( u32 frameIndex ) NOEXCEPT;

        static MW_NOTHROW u32  AcquireNextImage( u32 frameIndex ) NOEXCEPT;

        static MW_NOTHROW void BindGraphicsPipeline( u32 frameIndex, Ref<RHI::IGraphicsPipeline> hPipeline, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT;

        static MW_NOTHROW void BindDescriptors( u32 frameIndex, RHI::PipelineSignature pSignature, RHI::DescriptorHandle* pDescriptors, size_t descriptorCount, u32 firstSet, s32 MW_NULLABLE threadID = -1 );

		static MW_NOTHROW void SetDynamicViewports( u32 frameIndex, const RHI::Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT;
		static MW_NOTHROW void SetDynamicScissors( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT;
		static MW_NOTHROW void SetDynamicCullMode( u32 frameIndex, RHI::ECullMode cullMode ) NOEXCEPT;

		static MW_NOTHROW void SetDynamicViewportsST( u32 frameIndex, const RHI::Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID = -1 ) NOEXCEPT;
		static MW_NOTHROW void SetDynamicScissorsST( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID = -1 ) NOEXCEPT;
		static MW_NOTHROW void SetDynamicCullModeST( u32 frameIndex, RHI::ECullMode cullMode, s32 threadID = -1 ) NOEXCEPT;

		static MW_NOTHROW void DispatchCompute( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, RHI::DescriptorHandle* pDesciptors, size_t descriptorCount ) NOEXCEPT;
		static MW_NOTHROW void DispatchCompute2( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT;


        NODISCARD static u32  GetCurrentFrameIndex() NOEXCEPT { return m_CurrentFrameIndex; };
        NODISCARD static u32* GetCurrentFrameIndexPtr() NOEXCEPT { return &m_CurrentFrameIndex; };

		NODISCARD static u32  GetCurrentImageIndex() NOEXCEPT { return m_CurrentImageIndex; };
		NODISCARD static u32* GetCurrentImageIndexPtr() NOEXCEPT { return &m_CurrentImageIndex; };

        NODISCARD static Slang::ComPtr<slang::IGlobalSession> GetSlangGlobalSession() NOEXCEPT { return m_SlangGlobalSession; };

        static MW_NOTHROW void SetRenderableExtend( const SExtent2D* renderableExtent ) { m_RenderableExtent = *renderableExtent; };
        NODISCARD static MW_NOTHROW SExtent2D GetRenderableExtend() { return m_RenderableExtent; };
    private:
        static u32 m_CurrentFrameIndex;
        static u32 m_CurrentImageIndex;

        static SExtent2D m_RenderableExtent;

        static Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;

        static Ref<RHI::IGraphicsAPI> m_pInstance;
    };
}