#pragma once
#include <common/Base.hh>

#include <renderer/FrameGraph.hh>

namespace Monoworks 
{
	class CFrameManager 
	{
	public:
		static  void Init() NOEXCEPT;
		static  void Shutdown() NOEXCEPT; 

		static  void Render() NOEXCEPT;
		
		static  void SetFrameGraph( Ref<IFrameGraph>&& hFrameGraph ) NOEXCEPT { m_hCurrentFrameGraph = hFrameGraph; };

		NODISCARD static  Ref<IFrameGraph> GetCurrentFrameGraph() NOEXCEPT { return m_hCurrentFrameGraph; };
		NODISCARD static  u32& GetCurrentFrameIndex() NOEXCEPT { return m_CurrentFrameIndex; };
		NODISCARD static  u32& GetCurrentImageIndex() NOEXCEPT { return m_CurrentImageIndex; };

	private:
		static u32 m_CurrentFrameIndex;
		static u32 m_CurrentImageIndex;

		static Ref<IFrameGraph> m_hCurrentFrameGraph;
		
	};
}
