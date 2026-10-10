#pragma once
#include <common/Base.hh>

#include <renderer/FrameGraph.hh>

namespace Monoworks 
{
	class CFrameManager 
	{
	public:
		static MW_NOTHROW void Init() NOEXCEPT;
		static MW_NOTHROW void Shutdown() NOEXCEPT; 

		static MW_NOTHROW void Render( u32 frameIndex ) NOEXCEPT;
		
		static MW_NOTHROW void SetFrameGraph( Ref<IFrameGraph> hFrameGraph ) NOEXCEPT { m_hCurrentFrameGraph = hFrameGraph; };
		NODISCARD static MW_NOTHROW Ref<IFrameGraph> GetCurrentFrameGraph() NOEXCEPT { return m_hCurrentFrameGraph; };
	private:
		static Ref<IFrameGraph> m_hCurrentFrameGraph;
		
	};
}
