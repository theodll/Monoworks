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
	struct DefferedResolutionPassCreationInfo 
	{
		Ref<CShader> hShader;
	};

	class CDefferedResolutionPass 
	{
		CDefferedResolutionPass( DefferedResolutionPassCreationInfo* pInfo );
	
		// 1. reflection -> descriptor layout creation
		// 2. descriptor set allocation
		// 3. Buffer & texture upload

		void BindTexture( std::string_view name, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );
		void BindSampler( std::string_view name, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );
		void BindUBO(std::string_view name, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite = false );

	private:
		boost::unordered_map<u32, bool> m_BindingsWritten;
		RHI::DescriptorHandle m_pDescriptors[MFIF];
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		friend class CFrameGraph;
	};

	class CPostProcessPass 
	{
		Ref<CShader> hShader; 
	private:
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

