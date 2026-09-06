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
    class CStaticRenderer
    {
    public:
        static MW_NOTHROW void Init() NOEXCEPT;
        static MW_NOTHROW void Shutdown() NOEXCEPT;

        // Binds the descriptors for the root and all worker command buffers
        static MW_NOTHROW void BindStaticState( RHI::DescriptorHandle* pDescriptors, u32 descriptorCount ) NOEXCEPT;
        static MW_NOTHROW void BeginWorkerCommandbuffers() NOEXCEPT;
        static MW_NOTHROW void EndAndExecuteWorkerCommandbuffers() NOEXCEPT;

        MW_NOTHROW void BeginRendering( const RHI::BeginRenderingInfo* pInfo ) NOEXCEPT;
        MW_NOTHROW void EndRendering( ) NOEXCEPT;

        static MW_NOTHROW void DispatchCompute( Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, RHI::DescriptorHandle* pDesciptors, size_t pDescriptorCount ) NOEXCEPT;
		static MW_NOTHROW void DispatchCompute2( Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT;

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