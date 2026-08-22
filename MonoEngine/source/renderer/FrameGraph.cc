#include <common/Base.hh>

#include "FrameGraph.hh"

namespace Monoworks 
{
	CDefferedResolutionPass::CDefferedResolutionPass( const DefferedResolutionPassCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;



	};

	// 1. Pipeline Creation
	// 2. reflection -> descriptor layout creation
	// 3. descriptor set allocation
	// 4. Buffer & texture upload

	void CDefferedResolutionPass::BindTexture( std::string_view name, Ref<RHI::ITexture2D> hTexture )
	{
		MW_PROFILE_FUNC;

	};

	void CDefferedResolutionPass::BindUBO( std::string_view name, Ref<RHI::IUniformBuffer> hUniformBuffer ) 
	{
		MW_PROFILE_FUNC;

	};


}
