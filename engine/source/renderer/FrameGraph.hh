#pragma once
#include <common/Base.hh>

#include <boost/unordered_map.hpp>
#include <boost/compat/function_ref.hpp>
#include <boost/function.hpp>

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

	constexpr char c_GbufBindingName[] = "u_GBuffer";
	constexpr char c_GbufAOBindingName[] = "AlbedoOcclusion";
	constexpr char c_GbufNRMBindingName[] = "NormalRoughMetal";
	constexpr char c_GbufEBindingName[] = "Emissive";
	constexpr char c_GbufMVBindingName[] = "MotionVector";
	constexpr char c_GbufEMIDBindingName[] = "EntityMaterialID";
	constexpr char c_GbufDBindingName[] = "Depth";
	constexpr char c_GbufCBindingName[] = "Composite";
	constexpr char c_GbufSBindingName[] = "Sampler";

	enum ETonemapMode
	{
		MW_TONEMAP_MODE_NONE, // Pass through
		MW_TONEMAP_MODE_REINHARD,
		MW_TONEMAP_MODE_REINHARD_EXTENDED,
		MW_TONEMAP_MODE_REINHARD_JODIE,
		MW_TONEMAP_MODE_HABLE_FILMIC,
		MW_TONEMAP_MODE_ACES_FILMIC_APROX,
		MW_TONEMAP_MODE_AGX_APROX,
		MW_TONEMAP_MODE_KHRONOS_PBR_NEUTRAL
	};

	enum EAGXMode
	{
		MW_TONEMAP_AGX_MODE_DEFAULT,
		MW_TONEMAP_AGX_MODE_GOLDEN,
		MW_TONEMAP_AGX_MODE_PUNCHY
	};

	struct alignas(16) TonemapParams
	{
		ETonemapMode Mode;
		EAGXMode AGXMode; // only needed if using AGX
		float Exposure;
		bool EnableGamma;
	};

	/// @brief Interface to derive from when creating any sort of Frame Graph.
	class IFrameGraph 
	{
	public:
		virtual	~IFrameGraph() = default;

		/// @brief Pre rendering steps like binding global state, etc.
		virtual MW_NOTHROW void ExecutePreRenderingSteps( u32 frameIndex ) NOEXCEPT = 0;
		/// @brief Executes all Pre-Passes (Culling, Depth-Pre-Pass, ...)
		virtual MW_NOTHROW void ExecutePrePasses( u32 frameIndex  ) NOEXCEPT = 0;
		/// @brief Executes all Core-Passes (GPass, Deffered Resolution, ...)
		virtual MW_NOTHROW void ExecuteBasePasses( u32 frameIndex  ) NOEXCEPT = 0;
		/// @brief Executes all Post-Process-Passes (Bloom, Tone-Mapping, ...)
		virtual MW_NOTHROW void ExecutePostPasses( u32 frameIndex  ) NOEXCEPT = 0;

		virtual MW_NOTHROW void ExecutePostRenderingSteps( u32 frameIndex ) NOEXCEPT = 0;

		/// @brief Returns a reference to the current default Base-Pass-Pipeline
		virtual const MW_NOTHROW Ref<RHI::IGraphicsPipeline> GetDefaultBasePassPipeline() const = 0;
		/// @brief Returns a reference to the current default Base-Pass-Shader
		virtual const MW_NOTHROW Ref<CShader> GetDefaultBasePassShader() const = 0;

		virtual MW_NOTHROW void SetTonemapParams( TonemapParams* pParams ) NOEXCEPT = 0;
		virtual MW_NOTHROW void SetCamera( Ref<CCamera> hCamera ) NOEXCEPT = 0;
	
	};

	class IRenderPass 
	{
	public:
		IRenderPass( Ref<CShader> hShader, SExtent2D extent ) NOEXCEPT;
		virtual ~IRenderPass() NOEXCEPT = default;

		// Textures

		/**
		* @brief Bind a texture located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		virtual MW_NOTHROW void BindTexture( 
			std::string_view bindingName,
			Ref<RHI::ITexture2D> hTexture,
			bool forceRewrite = false,
			RHI::EDescriptorType imageType = RHI::MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) NOEXCEPT;

		/**
		* @brief Bind a texture for each frame in flight located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the textures to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		virtual MW_NOTHROW void BindTexture( 
			std::string_view bindingName,	
			std::span<Ref<RHI::ITexture2D>, MFIF> hTextures,
			bool forceRewrite = false, 
			RHI::EDescriptorType imageType = RHI::MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) NOEXCEPT;

		/**
		* @brief Bind a texture located in parameter block scope.
		* @param parameterBlockName: Parameter Block in which the binding is located
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		virtual MW_NOTHROW void BindTexture(
			std::string_view parameterBlockName,
			std::string_view bindingName,
			Ref<RHI::ITexture2D> hTexture,
			bool forceRewrite = false,
			RHI::EDescriptorType imageType = RHI::MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) NOEXCEPT;

		/**
		* @brief Bind a texture for each frame in flight located in parameter block scope.
		* @param parameterBlockName: Parameter Block in which the binding is located
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the textures to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		virtual MW_NOTHROW void BindTexture(
			std::string_view parameterBlockName,
			std::string_view bindingName,
			std::span<Ref<RHI::ITexture2D>, MFIF> hTextures,
			bool forceRewrite = false,
			RHI::EDescriptorType imageType = RHI::MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) NOEXCEPT;


		// UBOs 

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffer: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		virtual MW_NOTHROW void BindUBO(	 
			std::string_view bindingName,
			Ref<RHI::IUniformBuffer> hUniformBuffer,
			bool forceRewrite = false ) NOEXCEPT;

		/**
		* @brief Bind a uniform buffer for each frame in flight located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffers: Reference to the textures to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		virtual MW_NOTHROW void BindUBO(
			std::string_view bindingName,
			std::span<Ref<RHI::IUniformBuffer>, MFIF> hUniformBuffers,	
			bool forceRewrite = false ) NOEXCEPT;

		/**
		* @brief Bind a uniform buffer located in parameter block scope.
		* @param parameterBlockName: Parameter Block in which the binding is located
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffer: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		virtual MW_NOTHROW void BindUBO( 
			std::string_view parameterBlockName,
			std::string_view bindingName,
			Ref<RHI::IUniformBuffer> hUniformBuffer,
			bool forceRewrite = false ) NOEXCEPT;
		
		/**
		* @brief Bind a uniform buffer for each frame in flight located in parameter block scope.
		* @param parameterBlockName: Parameter Block in which the binding is located
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffers: Reference to the textures to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		virtual MW_NOTHROW void BindUBO( 
			std::string_view parameterBlockName,
			std::string_view bindingName,
			std::span<Ref<RHI::IUniformBuffer>, MFIF> hUniformBuffers,
			bool forceRewrite = false ) NOEXCEPT;

		// Samplers

		/**
		* @brief Bind a sampler located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		virtual MW_NOTHROW void BindSampler(
			std::string_view bindingName,
			Ref<RHI::ITexture2D> hSampler,
			bool forceRewrite = false ) NOEXCEPT;

		/**
		* @brief Bind a sampler for each frame in flight located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSamplers: Reference to the samplers to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		virtual MW_NOTHROW void BindSampler(
			std::string_view bindingName,
			std::span<Ref<RHI::ITexture2D>, MFIF> hSamplers,
			bool forceRewrite = false ) NOEXCEPT;

		/**
		* @brief Bind a sampler located in parameter block scope.
		* @param parameterBlockName: Parameter Block in which the binding is located
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		virtual MW_NOTHROW void BindSampler(
			std::string_view parameterBlockName,
			std::string_view bindingName,
			Ref<RHI::ITexture2D> hSampler,
			bool forceRewrite = false ) NOEXCEPT;

		/**
		* @brief Bind a sampler for each frame in flight located in parameter block scope.
		* @param parameterBlockName: Parameter Block in which the binding is located
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSamplers: Reference to the samplers to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		virtual MW_NOTHROW void BindSampler(
			std::string_view parameterBlockName,
			std::string_view bindingName,
			std::span<Ref<RHI::ITexture2D>, MFIF> hSamplers,
			bool forceRewrite = false ) NOEXCEPT;

	protected:
		boost::unordered_map<std::pair<u32, u32>, bool> m_BindingsWritten;
		std::array<std::vector<RHI::DescriptorHandle>, MFIF> m_pDescriptors; // One vector for each frame in flight so we can batch descriptor binding

		Ref<CShader> m_hShader;
		SExtent2D m_RenderingArea;
		u32 m_FirstSet;
	};

	enum EGraphicsRenderPassCreationFlagBits
	{
		MW_GRAPHICS_PRE_PASS_CREATION_FLAGS_NONE_BIT = 0x0,
		MW_GRAPHICS_PRE_PASS_CREATION_FLAGS_DISABLE_PIXEL_SHADER_BIT = 0b01
	};
	using EGraphicsRenderPassCreationFlags = flags_t;

	struct GraphicsRenderPassCreationInfo
	{
		// All of these are optional, a pipeline may not have any color attachments (like a depth-pre-pass-pipeline), but also may not have stencil or depth attachment.
		std::optional<std::span<std::span<RHI::RenderingAttachmentInfo>, MFIF>> MW_NULLABLE ppColorAttachments = std::nullopt;
		std::optional<std::span<RHI::RenderingAttachmentInfo, MFIF>>			MW_NULLABLE	pDepthAttachment = std::nullopt;
		std::optional<std::span<RHI::RenderingAttachmentInfo, MFIF>>			MW_NULLABLE	pStencilAttachment = std::nullopt;

		RHI::EImageFormat DepthFormat = RHI::MW_FORMAT_D32_SFLOAT; 
		RHI::EImageFormat StencilFormat = RHI::MW_FORMAT_UNDEFINED;

		std::optional<boost::compat::function_ref<void( u32 frameIndex )>>		MW_NULLABLE pExecutionScopeCallback = std::nullopt;

		Ref<CShader> hShader;
		SExtent2D RenderingArea;
		EGraphicsRenderPassCreationFlags Flags;
	};

	/**
	* @brief A Class to be inserted into the Frame Graph Pre-Pass section by either the Engine or a User.
	* This Data Structure takes in a Reference to a Shader, then reflects based on it and generates all needed descriptors and
	* creates a Graphics Pipeline.
	*
	* When writing a graphics-pre-pass Shader, all data the user wants to bind to must be in global scope, NOT in a ParameterBlock. This
	* is to allow easy binding, since parameters in the global scope are always set 0 in vulkan. If you need better performance - implement it yourself.
	*/
	class CGraphicsRenderPass final : public IRenderPass
	{
	public:
		CGraphicsRenderPass( const GraphicsRenderPassCreationInfo* pInfo );
		MW_NOTHROW ~CGraphicsRenderPass() NOEXCEPT;
		
		// Bind* functions are located in IRenderPass

	private:
		boost::function<void(u32 frameIndex)> MW_NULLABLE m_pExecutionScopeCallback = nullptr;

		std::optional<std::array<std::vector<RHI::RenderingAttachmentInfo>, MFIF>> m_ColorAttachments;
		std::optional<std::array<RHI::RenderingAttachmentInfo, MFIF>> m_DepthAttachment;
		std::optional<std::array<RHI::RenderingAttachmentInfo, MFIF>> m_StencilAttachment;

		Ref<RHI::IGraphicsPipeline> m_hGraphicsPipeline;
		Hash::hash_t m_PipelineHash;

		enum FlagBits
		{
			MW_GRAPHICS_PRE_PASS_USE_DEPTH_ATTACHMENT = 0b1,
			MW_GRAPHICS_PRE_PASS_USE_STENCIL_ATTACHMENT = 0b10,
		};
		using EGraphicsPrePassInternalFlags = flags_t;
		EGraphicsPrePassInternalFlags m_InternalFlags;

		friend class CDefferedFrameGraph;
	};

	struct ComputeRenderPassCreationInfo
	{
		Ref<CShader> hShader;
	};

	class CComputeRenderPass final : public IRenderPass
	{
	public:
		CComputeRenderPass( const ComputeRenderPassCreationInfo* pInfo );
		~CComputeRenderPass() NOEXCEPT;

		// Bind* functions are located in IRenderPass
	
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
		CDefferedFrameGraph( Ref<CCamera> hCamera )	NOEXCEPT;
		~CDefferedFrameGraph()	NOEXCEPT;

		MW_NOTHROW void ExecutePreRenderingSteps( u32 frameIndex ) NOEXCEPT override;
		/// @brief Executes all pre-passes.
		MW_NOTHROW void ExecutePrePasses( u32 frameIndex )	NOEXCEPT override;
		/// @brief Executes all core-passes.
		MW_NOTHROW void ExecuteBasePasses( u32 frameIndex ) NOEXCEPT override;
		/// @brief Executes all post & post-processing-passes.
		MW_NOTHROW void ExecutePostPasses( u32 frameIndex ) NOEXCEPT override;
		
		MW_NOTHROW void ExecutePostRenderingSteps( u32 frameIndex ) NOEXCEPT override;

		/**
		 * @brief Hooks a compute pre-pass into the frame graph.
		 * @param hComputePrePass Compute-Pass to be hooked into the frame-graph.
		 * @param executionPriority Priority of the Compute-Pre-Pass. Passing -1 as execution priority automatically inserts it in last place.
		 */
		MW_NOTHROW void AddPrePass(					Ref<CComputeRenderPass>  hComputePrePass,		s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;
		
		/**
		 * @brief Hooks a graphics pre-pass into the frame graph.
		 * @param hGraphicsPrePass Compute-Pass to be hooked into the frame-graph.
		 * @param executionPriority Priority of the Graphics-Pre-Pass. Passing -1 as execution priority automatically inserts it in last place.
		 */
		MW_NOTHROW void AddPrePass(					Ref<CGraphicsRenderPass> hGraphicsPrePass,		s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;

		/**  
		* @brief Hooks a deffered resolution pass into the frame graph.
		* @param hComputePrePass Compute-Pass to be hooked into the frame-graph.
		* @param executionPriority Priority of the deffered resolution pass. Passing -1 as execution priority automatically inserts it in last place.
		*/
		MW_NOTHROW void AddDefferedResolutionPass(	Ref<CComputeRenderPass> hComputePass,	s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;

		/**  
		* @brief Hooks a post process pass into the frame graph. 
		* @param hComputePrePass Compute - Pass to be hooked into the frame - graph.
		* @param executionPriority Priority of the deffered resolution pass. Passing -1 as execution priority automatically sets it in last place.
		*/
		MW_NOTHROW void AddPostProcessPass(			Ref<CComputeRenderPass> hPostProcessPass,		s32 MW_NULLABLE executionPriority = -1 ) NOEXCEPT;

		MW_NOTHROW const Ref<RHI::IGraphicsPipeline>	GetDefaultBasePassPipeline() const override { return m_hDefaultBasePassPipeline; };
		MW_NOTHROW const Ref<CShader>					GetDefaultBasePassShader() const override { return m_hDefaultBasePassShader; };

		MW_NOTHROW void SetCamera( Ref<CCamera> hCamera ) NOEXCEPT override { m_hCamera = hCamera; };
		MW_NOTHROW void SetTonemapParams( TonemapParams* pParams ) NOEXCEPT 
		{
			MW_PROFILE_FUNC;
			m_hTonemapParamsUBO->SetData( pParams, sizeof( TonemapParams ) );
		}

		struct alignas(16) ModelPushConstant
		{
			Matrix	CurrentModelMatrix;
			Matrix	PreviousModelMatrix;
			u32		EntityID = UINT32_MAX; // TODO: Implement
		};

	private:
		MW_NOTHROW void SubmitSceneGeometry( u32 frameIndex ) NOEXCEPT;
		MW_NOTHROW void SubmitDepthPrePassGeometry( u32 frameIndex ) NOEXCEPT;

		// NOTE: Execution Priority is the index of the array. E. g. Deffered Pass is at index 0 in m_hDefferedResolutionPasses.
		std::vector<Ref<CComputeRenderPass>>			m_hComputePrePasses;
		std::vector<Ref<CGraphicsRenderPass>>			m_hGraphicsPrePasses;
		std::vector<Ref<CComputeRenderPass>>	m_hDefferedResolutionPasses;
		std::vector<Ref<CComputeRenderPass>>			m_hPostProcessPasses;

		std::array<Ref<GBuffer>, MFIF> m_hGBuffers;
		std::array<RHI::DescriptorHandle, MFIF> m_hGBufferDescriptors = { nullptr }; // Bound after base pass at set number 1.

		// The composite image houses the gbuffer sampler.
		std::array<Ref<RHI::ITexture2D>, MFIF> m_hCompositeImages;
		std::array<Ref<RHI::ITexture2D>, MFIF> m_hPresentationProxyImages;

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

		std::array<Ref<RHI::IUniformBuffer>, MFIF> m_hCameraUBOs;
		std::array<RHI::DescriptorHandle, MFIF> m_hCameraUBOSets;
		Ref<CCamera> m_hCamera;

		Ref<RHI::IUniformBuffer> m_hTonemapParamsUBO;
		Ref<CPostProcessPass> m_hTonemapPass;

		


		// TEMP:


		Ref<CMesh> mesh;
	};
}

