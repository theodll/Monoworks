#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>

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
		CDefferedResolutionPass( const DefferedResolutionPassCreationInfo* pInfo );
	
		// 1. reflection -> descriptor layout creation
		// 2. descriptor set allocation
		// 3. Buffer & texture upload

		void BindTexture( std::string_view name, Ref<RHI::ITexture2D> hTexture );
		void BindUBO( std::string_view name, Ref<RHI::IUniformBuffer> hUniformBuffer);

	private:
		Ref<RHI::IComputePipeline> m_hComputePipeline;
		Ref<CShader> m_hShader;

		bool m_SamplerBound;

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

