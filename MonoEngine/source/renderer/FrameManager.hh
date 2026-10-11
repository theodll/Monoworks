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

		static MW_NOTHROW void Render() NOEXCEPT;
		
		static MW_NOTHROW void SetFrameGraph( Ref<IFrameGraph>&& hFrameGraph ) NOEXCEPT { m_hCurrentFrameGraph = hFrameGraph; };

		NODISCARD static MW_NOTHROW Ref<IFrameGraph> GetCurrentFrameGraph() NOEXCEPT { return m_hCurrentFrameGraph; };
		NODISCARD static MW_NOTHROW u32& GetCurrentFrameIndex() NOEXCEPT { return m_CurrentFrameIndex; };
		NODISCARD static MW_NOTHROW u32& GetCurrentImageIndex() NOEXCEPT { return m_CurrentImageIndex; };

	private:
		static u32 m_CurrentFrameIndex;
		static u32 m_CurrentImageIndex;

		static Ref<IFrameGraph> m_hCurrentFrameGraph;
		
	};
}
