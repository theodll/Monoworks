#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>

#include <rhi/GraphicsAPI.hh>
#include <rhi/Utils.hh>

#include <rhi/agnostic/DescriptorManager.hh>
#include <rhi/agnostic/Texture.hh>
#include <rhi/agnostic/UniformBuffer.hh>
#include <rhi/agnostic/ComputePipeline.hh>

#include <renderer/Camera.hh>
#include <renderer/Shader.hh>

namespace Monoworks 
{
	
	/// @brief Interface to derive from when creating any sort of Frame Graph.
	class IFrameGraph 
	{
	public:
		virtual	~IFrameGraph() = default;

		/// @brief Pre rendering steps like binding global state, etc.
		virtual MW_NOTHROW void ExecutePreRenderingSteps() NOEXCEPT = 0;
		/// @brief Executes all Pre-Passes (Culling, Depth-Pre-Pass, ...)
		virtual MW_NOTHROW void ExecutePrePasses() NOEXCEPT = 0;
		/// @brief Executes all Core-Passes (GPass, Deffered Resolution, ...)
		virtual MW_NOTHROW void ExecuteBasePasses() NOEXCEPT = 0;
		/// @brief Executes all Post-Process-Passes (Bloom, Tone-Mapping, ...)
		virtual MW_NOTHROW void ExecutePostPasses() NOEXCEPT = 0;

		virtual MW_NOTHROW void ExecutePostRenderingSteps() NOEXCEPT = 0;

		/// @brief Returns a reference to the current default Base-Pass-Pipeline
		virtual const MW_NOTHROW Ref<RHI::IGraphicsPipeline> GetDefaultBasePassPipeline() const = 0;
	};

	struct ComputePrePassCreationInfo
	{
		const Ref<CShader> hShader;
	};

	class CComputePrePass 
	{
		CComputePrePass( const ComputePrePassCreationInfo* pInfo );
		MW_NOTHROW ~CComputePrePass() NOEXCEPT;

		/**
		* @brief Bind a texture located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		void BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param phUniformBuffer: Array of References to the uniform buffers to bind for every frame in flight.
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer>* phUniformBuffers, bool forceRewrite = false );

	private:
		boost::unordered_map<u32, bool> m_BindingsWritten;
		RHI::DescriptorHandle m_pDescriptors[MFIF];
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		Hash::hash_t m_PipelineHash;

		friend class CDefferedFrameGraph;		friend class CDefferedFrameGraph;
	};

	struct GraphicsPrePassCreationInfo
	{
		Ref<CShader> hShader;
		SExtent2D RenderingArea;

		size_t ColorAttachmentCount = 0;
		std::array<RHI::RenderingAttachmentInfo*, MFIF>* MW_NULLABLE	ppColorAttachments = { nullptr };
		std::array<RHI::RenderingAttachmentInfo*, MFIF>  MW_NULLABLE	pDepthAttachment = { nullptr };
		std::array<RHI::RenderingAttachmentInfo*, MFIF>  MW_NULLABLE	pStencilAttachment = { nullptr };

		RHI::EImageFormat DepthFormat = RHI::MW_FORMAT_D32_SFLOAT; 
		RHI::EImageFormat StencilFormat = RHI::MW_FORMAT_S8_UINT;

		std::function<void( u32 frameIndex )> MW_NULLABLE pExecutionScopeCallback = nullptr;
	};

	/**
	* @brief A Class to be inserted into the Frame Graph Pre-Pass section by either the Engine or a User.
	* This Data Structure takes in a Reference to a Shader, then reflects based on it and generates all needed descriptors and
	* creates a Graphics Pipeline.
	*
	* When writing a graphics-pre-pass Shader, all data the user wants to bind to must be in global scope, NOT in a ParameterBlock. This
	* is to allow easy binding, since parameters in the global scope are always set 0 in vulkan. If you need better performance - implement it yourself.
	*/
	class CGraphicsPrePass
	{
		CGraphicsPrePass( const GraphicsPrePassCreationInfo* pInfo );
		MW_NOTHROW ~CGraphicsPrePass() NOEXCEPT;

		/**
		* @brief Bind a texture located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		void BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param phUniformBuffer:Array of References to the uniform buffers to bind for every frame in flight.
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer>* phUniformBuffers, bool forceRewrite = false );

	private:
		boost::unordered_map<u32, bool> m_BindingsWritten;
		RHI::DescriptorHandle m_pDescriptors[MFIF];
		Ref<RHI::IGraphicsPipeline> m_hGraphicsPipeline;
		// TODO: add the possibility to split this up. 
		
		Ref<CShader> m_hShader; // Note: all graphics shader stages in one slang module. 
		std::function<void(u32 frameIndex)> MW_NULLABLE m_pExecutionScopeCallback;

		enum FlagBits
		{
			MW_GRAPHICS_PRE_PASS_USE_DEPTH_ATTACHMENT,
			MW_GRAPHICS_PRE_PASS_USE_STENCIL_ATTACHMENT,
		};
		flags_t m_Flags;
		
		SExtent2D m_RenderingArea;
		std::vector<std::array<RHI::RenderingAttachmentInfo, MFIF>> m_ColorAttachments;
		
		std::array<RHI::RenderingAttachmentInfo, MFIF> m_DepthAttachment;
		std::array<RHI::RenderingAttachmentInfo, MFIF> m_StencilAttachment;

		Hash::hash_t m_PipelineHash;

		friend class CDefferedFrameGraph;
	};

	struct PostProcessPassCreationInfo 
	{
		const Ref<CShader> hShader;
	};

	/**
	 * @brief A Class to be inserted into the Frame Graph Post-Processing section by either the Engine or a User.
	 * This Data Structure takes in a Reference to a Shader, then reflects based on it and generates all needed descriptors and
	 * creates a Compute Pipeline. 
	 * 
	 * When writing a post-processing Shader, all data the user wants to bind to must be in global scope, NOT in a ParameterBlock. This
	 * is to allow easy binding, since parameters in the global scope are always set 0 in vulkan. If you need better performance - implement it yourself.
	 */
	class CPostProcessPass
	{
		CPostProcessPass( const PostProcessPassCreationInfo* pInfo );
		MW_NOTHROW ~CPostProcessPass() NOEXCEPT;

		/**
		* @brief Bind a texture located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		void BindTexture(  std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler(  std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param phUniformBuffer: Array of References to the uniform buffers to bind for every frame in flight.
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO(	   std::string_view bindingName, Ref<RHI::IUniformBuffer>* phUniformBuffers, bool forceRewrite = false );

	private:
		boost::unordered_map<u32, bool> m_BindingsWritten;
		RHI::DescriptorHandle m_pDescriptors[MFIF];
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		Hash::hash_t m_PipelineHash;

		friend class CDefferedFrameGraph;
	};

	struct DefferedResolutionPassCreationInfo
	{
		const Ref<CShader> hShader;
	};

	class CDefferedResolutionPass
	{
		CDefferedResolutionPass( const DefferedResolutionPassCreationInfo* pInfo );
		~CDefferedResolutionPass() NOEXCEPT;

		/**
		 * @brief Bind a texture located in a parameter block.
		 * @param parameterBlockName: Name of the parameter block in the shader code.
		 * @param bindingName: Name of the element to bind inside the parameter block.
		 * @param hTexture: Reference to the texture to bind.
		 * @param forceRewrite: Toggle whether to rewrite the texture if it's already written. 
		 */
		void BindTexture( std::string_view parameterBlockName,	std::string_view bindingName,	Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );

		/**
		* @brief Bind a texture located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		void BindTexture( std::string_view bindingName,			Ref<RHI::ITexture2D> hTexture,	bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in a parameter block.
		* @param parameterBlockName: Name of the parameter block in the shader code.
		* @param bindingName: Name of the element to bind inside the Parameter Block.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler( std::string_view parameterBlockName,	std::string_view bindingName,	Ref<RHI::ITexture2D> hSampler, bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler( std::string_view bindingName,			Ref<RHI::ITexture2D> hSampler,	bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in a parameter block.
		* @param parameterBlockName: Name of the parameter block in the shader code.
		* @param bindingName: Name of the element to bind inside the Parameter Block.
		* @param hUniformBuffer: Array of References to the uniform buffers to bind for every frame in flight.
		* @param forceRewrite: Toggle whether to rewrite the sampler uniform buffer if it's already written.
		*/
		void BindUBO(	  std::string_view parameterBlockName,	std::string_view bindingName,	Ref<RHI::IUniformBuffer>* phUniformBuffers, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param phUniformBuffer: Array of References to the uniform buffers to bind for every frame in flight.
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO(	  std::string_view bindingName,			Ref<RHI::IUniformBuffer>* phUniformBuffers,	bool forceRewrite = false );
	
	private:
		// NOTE: 1. element in the pair is the set binding and 2. is the in-set binding
		boost::unordered_map<std::pair<u32, u32>, bool> m_BindingsWritten;
		std::vector<std::array<RHI::DescriptorHandle, MFIF>> m_pDescriptors;
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		Hash::hash_t m_PipelineHash;

		friend class CDefferedFrameGraph;
	};

	// 26 Bytes per Pixel
	struct GBuffer
	{
		// For packing info, refer to the GPassBase Shader.
		Ref<RHI::ITexture2D> AlbedoOcclusion; // RGBA8UNORM
		Ref<RHI::ITexture2D> NormalRoughMetal; // A2BGR10UNORM 
		Ref<RHI::ITexture2D> Emissive; // RGB16UNORM
		Ref<RHI::ITexture2D> MotionVector; // RG16SFLOAT
		Ref<RHI::ITexture2D> EntityMaterialID; // R32UINT
		Ref<RHI::ITexture2D> Depth; // D32

		Ref<RHI::ITexture2D> Sampler;
 	};

	class CDefferedFrameGraph final : public IFrameGraph
	{
	public:
		CDefferedFrameGraph()	NOEXCEPT;
		~CDefferedFrameGraph()	NOEXCEPT;

		MW_NOTHROW void ExecutePreRenderingSteps() NOEXCEPT override;
		/// @brief Executes all pre-passes.
		MW_NOTHROW void ExecutePrePasses()	NOEXCEPT override;
		/// @brief Executes all core-passes.
		MW_NOTHROW void ExecuteBasePasses() NOEXCEPT override;
		/// @brief Executes all post & post-processing-passes.
		MW_NOTHROW void ExecutePostPasses() NOEXCEPT override;
		
		MW_NOTHROW void ExecutePostRenderingSteps() NOEXCEPT override; 

		/**
		 * @brief Hooks a compute pre-pass into the frame graph.
		 * @param hComputePrePass Compute-Pass to be hooked into the frame-graph.
		 * @param executionPriority Priority of the Compute-Pre-Pass. Passing -1 as execution priority automatically inserts it in last place.
		 */
		MW_NOTHROW void AddPrePass(					Ref<CComputePrePass>  hComputePrePass,		s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;
		
		/**
		 * @brief Hooks a graphics pre-pass into the frame graph.
		 * @param hGraphicsPrePass Compute-Pass to be hooked into the frame-graph.
		 * @param executionPriority Priority of the Graphics-Pre-Pass. Passing -1 as execution priority automatically inserts it in last place.
		 */
		MW_NOTHROW void AddPrePass(					Ref<CGraphicsPrePass> hGraphicsPrePass,		s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;

		/**  
		* @brief Hooks a deffered resolution pass into the frame graph.
		* @param hComputePrePass Compute-Pass to be hooked into the frame-graph.
		* @param executionPriority Priority of the deffered resolution pass. Passing -1 as execution priority automatically inserts it in last place.
		*/
		MW_NOTHROW void AddDefferedResolutionPass(	Ref<CDefferedResolutionPass> hComputePass,	s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;

		/**  
		* @brief Hooks a post process pass into the frame graph. 
		* @param hComputePrePass Compute - Pass to be hooked into the frame - graph.
		* @param executionPriority Priority of the deffered resolution pass. Passing -1 as execution priority automatically sets it in last place.
		*/
		MW_NOTHROW void AddPostProcessPass(			Ref<CPostProcessPass> hPostProcessPass,		s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;

		MW_NOTHROW const Ref<RHI::IGraphicsPipeline> GetDefaultBasePassPipeline() const override { return m_hDefaultBasePassPipeline; };

	private:
		// NOTE: Execution Priority is the index of the array. E. g. Deffered Pass is at index 0 in m_hDefferedResolutionPasses.
		std::vector<Ref<CComputePrePass>>			m_hComputePrePasses;
		std::vector<Ref<CGraphicsPrePass>>			m_hGraphicsPrePasses;
		std::vector<Ref<CDefferedResolutionPass>>	m_hDefferedResolutionPasses;
		std::vector<Ref<CPostProcessPass>>			m_hPostProcessPasses;

		std::array<Ref<GBuffer>, MFIF> m_hGBuffers;
		std::array<RHI::DescriptorHandle, MFIF> m_hGBufferDescriptor = { nullptr }; // Bound after base pass at set number 1.

		std::array<Ref<RHI::ITexture2D>, MFIF> m_hCompositeImage;

		Ref<CShader> m_hDefaultBasePassShader; // NOTE: Fragment and Vertex Shader
		Ref<RHI::IGraphicsPipeline>	m_hDefaultBasePassPipeline;
		Hash::hash_t			m_DefaultBasePassPipelineHash;

		struct alignas( 16 ) CameraConstantsUBO
		{
			Matrix CurrentViewProjection;
			Matrix PreviousViewProjection;
			Matrix CurrentInverseViewProjection;
			Matrix PreviousInverseViewProjection;
			Vector CameraPosition;
			int _pad0;
		};

		std::array<Ref<RHI::IUniformBuffer>, MFIF> m_CameraUBOs;
		std::array<RHI::DescriptorHandle, MFIF> m_CameraUBOSets;
		Ref<CCamera> m_hCamera;
	};
}

