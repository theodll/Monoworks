#include <mwpch.hh>

#include <core/Application.hh>

#include <renderer/StaticRenderer.hh>
#include <renderer/FrameManager.hh>

namespace Monoworks 
{


	Ref<IFrameGraph> CFrameManager::m_hCurrentFrameGraph;

	MW_NOTHROW void CFrameManager::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		SCVar executePrepasses{};
		executePrepasses.Name = "r_execute_pre_passes";
		executePrepasses.Value = 1.0f;
		MW_REG_CVAR( &executePrepasses );

		SCVar executePostPasses{};
		executePostPasses.Name = "r_execute_post_passes";
		executePostPasses.Value = 1.0f;
		MW_REG_CVAR( &executePostPasses );


	}


	MW_NOTHROW void CFrameManager::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CFrameManager::Render( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// Pre rendering setup
		CStaticRenderer::AcquireNextImage( frameIndex );
		
		CStaticRenderer::BeginRootCommandbuffer( frameIndex );

		auto presenter = CApplication::GetCreateInfos()->pPresenter;
		auto currentSwapchainImage = presenter->GetSwapchainImages()[frameIndex];

		currentSwapchainImage->TransitionLayout( frameIndex, RHI::MW_IMAGE_LAYOUT_GENERAL, RHI::MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, RHI::MW_IMAGE_ASPECT_COLOR_BIT );

		if ( m_hCurrentFrameGraph )
		{

			m_hCurrentFrameGraph->ExecutePreRenderingSteps( frameIndex );

			if ( CCvarManager::GetValue( "r_execute_pre_passes" ) != 0.0f )
				m_hCurrentFrameGraph->ExecutePrePasses( frameIndex );

			m_hCurrentFrameGraph->ExecuteBasePasses( frameIndex );

			if ( CCvarManager::GetValue( "r_execute_post_passes" ) != 0.0f )
				m_hCurrentFrameGraph->ExecutePostPasses( frameIndex );

			m_hCurrentFrameGraph->ExecutePostRenderingSteps( frameIndex );

		}

		currentSwapchainImage->TransitionLayout( frameIndex, RHI::MW_IMAGE_LAYOUT_PRESENT_SRC_KHR, RHI::MW_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, RHI::MW_IMAGE_ASPECT_COLOR_BIT );

		CStaticRenderer::SubmitRootCommandbuffer( frameIndex );

		CStaticRenderer::Present( frameIndex );

	}



}
