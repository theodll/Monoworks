#pragma once
#include <common/Base.hh>

#include "GraphicsPipeline.hh"

#include <boost/unordered/unordered_map.hpp>

namespace Monoworks::RHI 
{
	class CPipelineManager 
	{
	public:
		static void Init();
		static void Shutdown();

	private:
		// TODO: UUID  
		boost::unordered::unordered_map<u64, Ref<IGraphicsPipeline>> m_GraphicPipelines;
	};
}
