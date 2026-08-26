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
	// set 0 -> global scope
	// set 1 -> gbuffer ( always! even when not using the gbuffer but using a following parameter block, the gbuffer has to be there. )
	// set 2... -> user defined

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
		void BindTexture( std::string_view parameterBlockName,	std::string_view bindingName,	Ref<RHI::ITexture> hTexture, bool forceRewrite = false );

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
		void BindSampler( std::string_view parameterBlockName,	std::string_view bindingName,	Ref<RHI::ITexture> hSampler, bool forceRewrite = false );

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

	class CFrameGraph 
	{
	public:
		CFrameGraph() NOEXCEPT;
		~CFrameGraph() NOEXCEPT;

		/**
		 * @brief Hooks a user specified deffered resolution pass into the frame graph.
		 */
		void AddDefferedResolutionPass( Ref<CDefferedResolutionPass> hComputePass, u32 MW_NULLABLE executionPriority = UINT32_MAX );
		void AddPostProcessPass( Ref<CPostProcessPass> hPostProcessPass, u32 MW_NULLABLE executionPriority = UINT32_MAX );

	private:
		// NOTE: Execution Priority is the index of the array. E. g. Deffered Pass is at index 0 in m_hDefferedResolutionPasses.
		std::vector<Ref<CDefferedResolutionPass>>	m_hDefferedResolutionPasses;
		std::vector<Ref<CPostProcessPass>>		m_hPostProcessPasses;

	};
}

