#pragma once
#include <common/Base.hh>
#include <common/Events.hh>
#include <events/Event.hh>
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
		/**
		 * @brief Initializes the Rendering Hardware Interface and Graphics Context.
		 * API of the Graphics Context is decided by the Config File "
		 */
		static void MWAPI_CALL Init() NOEXCEPT;

		/**
		 * @brief Shuts down the Graphics Context and Rendering Hardware Interface.
		 * All VMA Allocated Memory (Textures, Vertex-, Index- and Uniform Buffers) must be released before this point. Otherwise it will assert and crash.
		 */
		static void MWAPI_CALL Shutdown() NOEXCEPT;
		
		/// @brief Writes all Graphics Frame Profiling Data into a FrameGraphicsProfilingData struct and Profiler.
		static void MWAPI_CALL ProfileFrameData() NOEXCEPT;
		
		/**
		 * @brief Begins root commandbuffer recording of frame index frameIndex.
		 * @param frameIndex frame index of the current frame.
		
		*/
		static void MWAPI_CALL BeginRootCommandbuffer( u32 frameIndex ) NOEXCEPT;
		
		/**
		 * @brief Ends root commandbuffer recording of frame index frameIndex.
		 * @param frameIndex frame index of the current frame.
		 * Must not be called before BeginRootCommandbuffer has been called at the equal frame index.
		 */
		static void MWAPI_CALL SubmitRootCommandbuffer( u32 frameIndex );
		
		/**
		 * @brief Blocks until Rendering and the GPU is idle.
		 * Should not be called inside a Job or continuous loop.
		 */
		static void MWAPI_CALL WaitRenderingIdle() NOEXCEPT;
		
		/**
		 * @brief Begins recording of each commandbuffer for every worker thread.
		 * @param frameIndex frame index of the current frame.
		 */
		static void MWAPI_CALL BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT;
	
		/**
		 * @brief Ends recording of each commandbuffer for every worker thread and merges them into the root commandbuffer.
		 * @param frameIndex frame index of the current frame.
		 * Should optimally be called before EndRendering when doing submission work inside a vk*Rendering scope on secondary commandbuffers.
		 * Must not be called before BeginSecondaryCommandbuffers has been called at the equal frame index.
		 */
		static void MWAPI_CALL MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT;
		
		/**
		 * @brief Begins a rendering scope on frame index frameIndex.
		 * @param frameIndex frame index of the current frame.
		 * Must not be called before BeginRootCommandbuffer has been called at the equal frame index.
		 */
		static void MWAPI_CALL BeginRendering( 
			u32 frameIndex,
			const RHI::BeginRenderingInfo* pInfo ) NOEXCEPT;

		/**
		 * @brief Ends the latest rendering scope on frame index frameIndex.
		 * @param frameIndex frame index of the current frame.
		 * Must not be called before BeginRootCommandbuffer has been called at the equal frame index.
		 * Must not be called before BeginRendering has been called at least once without matching end, leaving a rendering scope open, at the equal frame index.
		 */
		static void MWAPI_CALL EndRendering( u32 frameIndex ) NOEXCEPT;

		/**
		 * @brief Acquires the next image from the selected IPresentation interface.
		 * @return The matching image index to the presentation image.
		 * @param frameIndex frame index of the current frame.
		 */
		static EResult MWAPI_CALL AcquireNextImage( u32 frameIndex, u32* pImageIndex ) NOEXCEPT;

		/**
		 * @brief Presents the current, from the IPresentation acquired, presentation image through the selected IPresentation interface.
		 * @param frameIndex frame index of the current frame.
		 * Must not be called before AcuireNextImage has been called for the current frame index and the root commandbuffer has been submitted to the graphics queue.
		 */
		static void MWAPI_CALL Present(	u32 frameIndex ) NOEXCEPT;

		/**
		 * @brief Binds the graphics pipeline specified at hPipeline.
		 * @param frameIndex frame index of the current frame.
		 * @param threadID ID of the current thread. 
		 * If threadID < 0, the graphics pipeline is bound to the root command buffer
		 */
		static void MWAPI_CALL BindGraphicsPipeline(	
			u32 frameIndex,
			Ref<RHI::IGraphicsPipeline> hPipeline,
			s32 threadID = -1 ) NOEXCEPT;

		/**
		 * @brief Binds all descriptors in the pDescriptors array.
		 * @param frameIndex frame index of the current frame.
		 * @param pSignature signature of the bound pipeline.
		 * @param ppDescriptors array of all descriptors to bind.
		 * @param firstSet Binding index of the first descriptor specified in ppDescriptors
		 * @param threadID ID of the current thread.
		 * @param bindpoint bind point of where to bind the descriptors 
		 * If threadID < 0, the descriptors is bound to the root command buffer
		 */
		static void MWAPI_CALL BindDescriptors(	
			u32 frameIndex,
			RHI::PipelineSignature pSignature,
			std::span<RHI::DescriptorHandle> ppDescriptors,
			u32 firstSet,
			s32 threadID = -1,
			RHI::EPipelineBindPoint bindpoint = RHI::MW_PIPELINE_BIND_POINT_BOTH );

		/**
				 * @brief Draws a static indexed mesh with an optional transformation and pipeline override.
				 * @param frameIndex frame index of the current frame.
				 * @param hMesh reference to the mesh to be drawn.
				 * @param transform transformation matrix applied to the mesh.
				 * @param threadID ID of the current thread. If threadID < 0, it is recorded to the root command buffer.
				 * @param hPipelineOverride optional pipeline override to use instead of the mesh's default pipeline.
				 */
		static void MWAPI_CALL DrawStaticMeshIndexed(
			u32 frameIndex,
			Ref<CMesh> hMesh,
			const Matrix& transform,
			s32 threadID = -1,
			Ref<RHI::IGraphicsPipeline> hPipelineOverride = nullptr ) NOEXCEPT;

		/**
		 * @brief Sets dynamic viewports for the root command buffer.
		 * @param frameIndex frame index of the current frame.
		 * @param pViewports span of viewports to set.
		 * @param firstViewport index of the first viewport to modify.
		 */
		static void MWAPI_CALL SetDynamicViewports(
			u32 frameIndex,
			std::span<const RHI::Viewport> pViewports,
			size_t firstViewport ) NOEXCEPT;

		/**
		 * @brief Sets dynamic scissor rectangles for the root command buffer.
		 * @param frameIndex frame index of the current frame.
		 * @param pScissors span of scissor extents to set.
		 * @param firstScissor index of the first scissor to modify.
		 */
		static void MWAPI_CALL SetDynamicScissors(
			u32 frameIndex,
			std::span<const SExtent2D> pScissors,
			size_t firstScissor ) NOEXCEPT;

		/**
		 * @brief Sets the dynamic cull mode for the root command buffer.
		 * @param frameIndex frame index of the current frame.
		 * @param cullMode the cull mode to apply.
		 */
		static void MWAPI_CALL SetDynamicCullMode(
			u32 frameIndex,
			RHI::ECullMode cullMode ) NOEXCEPT;

		/**
		 * @brief Sets dynamic viewports for a specific worker thread or root command buffer.
		 * @param frameIndex frame index of the current frame.
		 * @param pViewports span of viewports to set.
		 * @param firstViewport index of the first viewport to modify.
		 * @param threadID ID of the current thread. If threadID < 0, it applies to the root command buffer.
		 */
		static void MWAPI_CALL SetDynamicViewportsST(
			u32 frameIndex,
			std::span<const RHI::Viewport> pViewports,
			size_t firstViewport,
			s32 threadID = -1 ) NOEXCEPT;

		/**
		 * @brief Sets dynamic scissor rectangles for a specific worker thread or root command buffer.
		 * @param frameIndex frame index of the current frame.
		 * @param pScissors span of scissor extents to set.
		 * @param firstScissor index of the first scissor to modify.
		 * @param threadID ID of the current thread. If threadID < 0, it applies to the root command buffer.
		 */
		static void MWAPI_CALL SetDynamicScissorsST(
			u32 frameIndex,
			std::span<const SExtent2D> pScissors,
			size_t firstScissor,
			s32 threadID = -1 ) NOEXCEPT;

		/**
		 * @brief Sets the dynamic cull mode for a specific worker thread or root command buffer.
		 * @param frameIndex frame index of the current frame.
		 * @param cullMode the cull mode to apply.
		 * @param threadID ID of the current thread. If threadID < 0, it applies to the root command buffer.
		 */
		static void MWAPI_CALL SetDynamicCullModeST(
			u32 frameIndex,
			RHI::ECullMode cullMode,
			s32 threadID = -1 ) NOEXCEPT;

		/**
		 * @brief Dispatches a compute shader workload with descriptor bindings.
		 * @param frameIndex frame index of the current frame.
		 * @param hPipeline reference to the compute pipeline to dispatch.
		 * @param workgroup workgroup dimensions (X, Y, Z).
		 * @param pDesciptors span of descriptors to bind for the compute pass.
		 * @param threadID ID of the current thread. If threadID < 0, it applies to the root command buffer.
		 */
		static void MWAPI_CALL DispatchCompute(
			u32 frameIndex,
			Ref<RHI::IComputePipeline> hPipeline,
			Vector workgroup,
			std::span<RHI::DescriptorHandle> pDesciptors,
			u32 firstDescriptor,
			s32 threadID = -1 ) NOEXCEPT;

		/**
		 * @brief Dispatches a compute shader workload without explicit inline descriptor binding arrays.
		 * @param frameIndex frame index of the current frame.
		 * @param hPipeline reference to the compute pipeline to dispatch.
		 * @param workgroup workgroup dimensions (X, Y, Z).
		 * @param threadID ID of the current thread. If threadID < 0, it applies to the root command buffer.
		 */
		static void MWAPI_CALL DispatchCompute2(
			u32 frameIndex,
			Ref<RHI::IComputePipeline> hPipeline,
			Vector workgroup,
			s32 threadID = -1 ) NOEXCEPT;

		/// @brief Gets the fallback 2D texture used when an albedo texture is missing.
		NODISCARD static Ref<RHI::ITexture2D>					MWAPI_CALL GetDefaultMissingTexture() { return m_hMissingTexture; };

		/// @brief Gets the fallback 2D texture used when utility maps (such as normal or emissive maps) are missing.
		NODISCARD static Ref<RHI::ITexture2D>					MWAPI_CALL GetDefaultMissingMap() { return m_hMissingMap; };

		/// @brief Gets the global Slang compiler session instance.
		NODISCARD static Slang::ComPtr<slang::IGlobalSession>	MWAPI_CALL GetSlangGlobalSession() NOEXCEPT { return m_SlangGlobalSession; };

		/// @brief Gets the current renderable surface extent.
		NODISCARD static SExtent2D								MWAPI_CALL GetRenderableExtend() { return m_RenderableExtent; };

		/// @brief Gets a pointer to the current frame's graphics profiling data structure.
		NODISCARD static FrameGraphicsProfilingData* MWAPI_CALL GetFrameGraphicsProfilingData() { return &m_GraphicsProfilingData; };

		/**
		 * @brief Sets the renderable surface extent.
		 * @param renderableExtent pointer to the new renderable extent dimensions.
		 */
		static void MWAPI_CALL SetRenderableExtend( const SExtent2D& renderableExtent ) { m_RenderableExtent = renderableExtent; };

		static bool MWAPI_CALL OnResize( SEvent& e );

		static bool MWAPI_CALL IsMinimized() { return m_bMinimized; };

    private:
        // Used for albedo textures 
        static Ref<RHI::ITexture2D> m_hMissingTexture;
        // Used for missing maps like missing normal maps, emissive maps etc.
        static Ref<RHI::ITexture2D> m_hMissingMap;

        static FrameGraphicsProfilingData m_GraphicsProfilingData;

        static SExtent2D m_RenderableExtent;

		static bool m_bMinimized;

        static Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;

        static Ref<RHI::IGraphicsAPI> m_pInstance;
    };
}