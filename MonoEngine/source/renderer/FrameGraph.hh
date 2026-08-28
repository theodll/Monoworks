#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>

#include <rhi/agnostic/DescriptorManager.hh>
#include <rhi/agnostic/Texture.hh>
#include <rhi/agnostic/UniformBuffer.hh>
#include <rhi/agnostic/ComputePipeline.hh>

#include "Shader.hh"

namespace Monoworks 
{
	
	/// @brief Interface to derive from when creating any sort of Frame Graph.
	class IFrameGraph 
	{
	public:
		virtual	~IFrameGraph() = default;
		
		// @brief  Executes all passes
		virtual MW_NOTHROW void Execute() NOEXCEPT = 0;
		/// @brief Executes all Pre-Passes (Culling, Depth-Pre-Pass, ...)
		virtual MW_NOTHROW void ExecutePrePasses() NOEXCEPT = 0;
		/// @brief Executes all Core-Passes (GPass, Deffered Resolution, ...)
		virtual MW_NOTHROW void ExecuteCorePasses() NOEXCEPT = 0;
		/// @brief Executes all Post-Process-Passes (Bloom, Tone-Mapping, ...)
		virtual MW_NOTHROW void ExecutePostPasses() NOEXCEPT = 0;
	};

	class CComputePrePass 
	{
		// TODO: Implement
	};

	class CGraphicsPrePass
	{
		// TODO: Implement
	};

	struct PostProcessPassCreationInfo 
	{
		Ref<CShader> hShader;
	};

	/**
	 * @brief A Class to be inserted into the Frame Graph by either the Engine or a User.
	 * This Data Structure takes in a Reference to a Shader, then reflects based on it and generates all needed descriptors and
	 * creates a Compute Pipeline. 
	 * 
	 * When writing a post-processing Shader, all data the user wants to bind to must be in global scope, NOT in a ParameterBlock. This
	 * is to allow easy binding, since parameters in the global scope are always set 0 in vulkan. If you need better performance - implement it yourself.
	 */
	class CPostProcessPass
	{
		CPostProcessPass( PostProcessPassCreationInfo* pInfo );
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
		* @param hUniformBuffer: Reference to the uniform buffer to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO(	   std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite = false );

	private:
		boost::unordered_map<u32, bool> m_BindingsWritten;
		RHI::DescriptorHandle m_pDescriptors[MFIF];
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		Hash::hash_t m_PipelineHash;

		friend class CFrameGraph;
	};

	struct DefferedResolutionPassCreateionInfo
	{
		Ref<CShader> hShader;
	};

	class CDefferedResolutionPass
	{
		CDefferedResolutionPass( DefferedResolutionPassCreateionInfo* pInfo );
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
		* @param hUniformBuffer: Reference to the uniform buffer to bind.
		* @param forceRewrite: Toggle whether to rewrite the sampler uniform buffer if it's already written.
		*/
		void BindUBO(	  std::string_view parameterBlockName,	std::string_view bindingName,	Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffer: Reference to the uniform buffer to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO(	  std::string_view bindingName,			Ref<RHI::IUniformBuffer> hUniformBuffer,	bool forceRewrite = false );
	
	private:
		// NOTE: 1. element in the pair is the set binding and 2. is the in-set binding
		boost::unordered_map<std::pair<u32, u32>, bool> m_BindingsWritten;
		std::vector<std::array<RHI::DescriptorHandle, MFIF>> m_pDescriptors;
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		Hash::hash_t m_PipelineHash;

		friend class CFrameGraph;
	};

	class CDefferedFrameGraph final : public IFrameGraph
	{
	public:
		CDefferedFrameGraph()	NOEXCEPT;
		~CDefferedFrameGraph()	NOEXCEPT;

		MW_NOTHROW void Execute() NOEXCEPT override; 
		/// @brief Executes all pre-passes.
		MW_NOTHROW void ExecutePrePasses()	NOEXCEPT override;
		/// @brief Executes all core-passes.
		MW_NOTHROW void ExecuteCorePasses() NOEXCEPT override;
		/// @brief Executes all post & post-processing-passes.
		MW_NOTHROW void ExecutePostPasses() NOEXCEPT override;

		/**
		 * @brief Hooks a compute pre-pass into the frame graph.
		 * @param hComputePrePass Compute-Pass to be hooked into the frame-graph.
		 * @param executionPriority Priority of the Compute-Pre-Pass 
		 */
		MW_NOTHROW void AddPrePass(					Ref<CComputePrePass>  hComputePrePass,		u32 MW_NULLABLE executionPriority = UINT32_MAX ) NOEXCEPT;
		
		/**
		 * @brief Hooks a graphics pre-pass into the frame graph.
		 * @param hGraphicsPrePass Compute-Pass to be hooked into the frame-graph.
		 * @param executionPriority Priority of the Graphics-Pre-Pass
		 */
		MW_NOTHROW void AddPrePass( Ref<CGraphicsPrePass> hGraphicsPrePass, u32 MW_NULLABLE executionPriority = UINT32_MAX ) NOEXCEPT;

		/**  
		* @brief Hooks a deffered resolution pass into the frame graph.
		* @param hComputePrePass Compute-Pass to be hooked into the frame-graph.
		* @param executionPriority Priority of the deffered resolution pass
		*/
		MW_NOTHROW void AddDefferedResolutionPass(	Ref<CDefferedResolutionPass> hComputePass,	u32 MW_NULLABLE executionPriority = UINT32_MAX ) NOEXCEPT;

		/**  
		* @brief Hooks a post process pass into the frame graph. 
		* @param hComputePrePass Compute - Pass to be hooked into the frame - graph.
		* @param executionPriority Priority of the deffered resolution pass
		*/
		MW_NOTHROW void AddPostProcessPass(			Ref<CPostProcessPass> hPostProcessPass,		u32 MW_NULLABLE executionPriority = UINT32_MAX ) NOEXCEPT;

	private:
		// NOTE: Execution Priority is the index of the array. E. g. Deffered Pass is at index 0 in m_hDefferedResolutionPasses.
		std::vector<Ref<CComputePrePass>>			m_hComputePrePasses;
		std::vector<Ref<CGraphicsPrePass>>			m_hGraphicsPrePasses;
		std::vector<Ref<CDefferedResolutionPass>>	m_hDefferedResolutionPasses;
		std::vector<Ref<CPostProcessPass>>			m_hPostProcessPasses;

	};
}

