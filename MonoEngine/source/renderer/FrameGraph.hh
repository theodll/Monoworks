#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>

#include <rhi/agnostic/Texture.hh>

#include "Shader.hh"

namespace Monoworks 
{
	class CDefferedResolvePass 
	{
		Ref<CShader> hShader;

		void Set( u32 binding, Ref<RHI::ITexture2D> hTexture );
		void Set( u32 binding, const Vector* pVector );

	private:
		boost::unordered_map<u32, Ref<RHI::ITexture2D>> hTextures;
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
		void AddDefferedResolutionPass( Ref<CDefferedResolvePass> hComputePass, u32 MW_NULLABLE executionPriority = UINT32_MAX );
		void AddPostProcessPass( Ref<CPostProcessPass> hPostProcessPass, u32 MW_NULLABLE executionPriority = UINT32_MAX );

	private:
		// NOTE: Execution Priority is the index of the array. E. g. Deffered Pass is at index 0 in m_hDefferedResolutionPasses.
		std::vector<Ref<CDefferedResolvePass>>	m_hDefferedResolutionPasses;
		std::vector<Ref<CPostProcessPass>>		m_hPostProcessPasses;

	};
}

