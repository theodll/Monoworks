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
    struct FrameGraphicsProfilingData
    {
        u32 DrawCallCount;
    };
	
    // For Documentation of undocumented parts, refer to IGraphicsAPI.
    class CStaticRenderer
    {
    public:
        static  void Init() NOEXCEPT;
        static  void Shutdown() NOEXCEPT;

        static  void ProfileFrameData() NOEXCEPT;

        static  void BeginRootCommandbuffer( u32 frameIndex ) NOEXCEPT;
        static void SubmitRootCommandbuffer( u32 frameIndex );

        static void WaitRenderingIdle() NOEXCEPT;

		static  void BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT;
		static  void MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT;

		static  void BeginRendering( u32 frameIndex, const RHI::BeginRenderingInfo* pInfo ) NOEXCEPT;
		static  void EndRendering( u32 frameIndex ) NOEXCEPT;

        static  u32  AcquireNextImage( u32 frameIndex ) NOEXCEPT;
        static  void Present( u32 frameIndex ) NOEXCEPT;

        static  void BindGraphicsPipeline( u32 frameIndex, Ref<RHI::IGraphicsPipeline> hPipeline, s32 threadID = -1 ) NOEXCEPT;

		static  void BindDescriptors( u32 frameIndex, RHI::PipelineSignature pSignature, RHI::DescriptorHandle* pDescriptors, size_t descriptorCount, u32 firstSet, s32 threadID = -1, RHI::EPipelineBindPoint bindpoint = RHI::MW_PIPELINE_BIND_POINT_BOTH );

        static  void DrawStaticMeshIndexed( u32 frameIndex, Ref<CMesh> hMesh, const Matrix& transform, s32 threadID = -1, Ref<RHI::IGraphicsPipeline> hPipelineOverride = nullptr ) NOEXCEPT;

		static  void SetDynamicViewports( u32 frameIndex, const RHI::Viewport* pViewports, size_t viewportCount, size_t firstViewport ) NOEXCEPT;
		static  void SetDynamicScissors( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor ) NOEXCEPT;
		static  void SetDynamicCullMode( u32 frameIndex, RHI::ECullMode cullMode ) NOEXCEPT;

		static  void SetDynamicViewportsST( u32 frameIndex, const RHI::Viewport* pViewports, size_t viewportCount, size_t firstViewport, s32 threadID = -1 ) NOEXCEPT;
		static  void SetDynamicScissorsST( u32 frameIndex, const SExtent2D* pScissors, size_t scissorCount, size_t firstScissor, s32 threadID = -1 ) NOEXCEPT;
		static  void SetDynamicCullModeST( u32 frameIndex, RHI::ECullMode cullMode, s32 threadID = -1 ) NOEXCEPT;

		static  void DispatchCompute( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, std::span<RHI::DescriptorHandle> pDesciptors, s32 threadID = -1 ) NOEXCEPT;
		static  void DispatchCompute2( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 threadID = -1 ) NOEXCEPT;

        NODISCARD static Ref<RHI::ITexture2D> GetDefaultMissingTexture() { return m_hMissingTexture; };
        NODISCARD static Ref<RHI::ITexture2D> GetDefaultMissingMap() { return m_hMissingMap; };

        NODISCARD static Slang::ComPtr<slang::IGlobalSession> GetSlangGlobalSession() NOEXCEPT { return m_SlangGlobalSession; };

        static  void SetRenderableExtend( const SExtent2D* renderableExtent ) { m_RenderableExtent = *renderableExtent; };
        NODISCARD static  SExtent2D GetRenderableExtend() { return m_RenderableExtent; };

        NODISCARD static  FrameGraphicsProfilingData* GetFrameGraphicsProfilingData() { return &m_GraphicsProfilingData; };

    private:
        // Used for albedo textures 
        static Ref<RHI::ITexture2D> m_hMissingTexture;
        // Used for missing maps like missing normal maps, emissive maps etc.
        static Ref<RHI::ITexture2D> m_hMissingMap;

        static FrameGraphicsProfilingData m_GraphicsProfilingData;

        static SExtent2D m_RenderableExtent;

        static Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;

        static Ref<RHI::IGraphicsAPI> m_pInstance;
    };
}