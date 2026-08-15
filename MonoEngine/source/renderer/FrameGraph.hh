#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	struct GraphicsPass
	{
	};

	struct DefferedResolvePass 
	{
		u32 ExecutionPriority;
	};

	struct PostProcessPass 
	{
	};

	class CFrameGraph 
	{
	public:
		CFrameGraph() NOEXCEPT;
		~CFrameGraph() NOEXCEPT;

		void SetGraphicsPass( Ref<GraphicsPass> hGraphicsPass );
		void AddComputePass( Ref<DefferedResolvePass> hComputePass );
		void AddPostProcessPass( Ref<PostProcessPass> hPostProcessPass );

	private:
		std::vector<Ref<DefferedResolvePass>>	m_hDefferedResolvePasses;
		std::vector<Ref<PostProcessPass>>		m_hPostProcessPasses;
		Ref<GraphicsPass>						m_hGraphicsPass;

	};
}

