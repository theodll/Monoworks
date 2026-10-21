#include <mwpch.hh>

#include <core/Application.hh>

#include <renderer/StaticRenderer.hh>
#include <renderer/FrameManager.hh>

namespace Monoworks 
{


	u32 CFrameManager::m_CurrentFrameIndex;
	u32 CFrameManager::m_CurrentImageIndex;

	Ref<IFrameGraph> CFrameManager::m_hCurrentFrameGraph;

	 void CFrameManager::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_INFO("Initialize CFrameManager");

		SCVar* executePrepasses = new SCVar;
		executePrepasses->Name = "r_execute_pre_passes";
		executePrepasses->Value = 1.0f;
		CCvarManager::RegisterVariable( executePrepasses );
		
		SCVar* executePostPasses = new SCVar;
		executePostPasses->Name = "r_execute_post_passes";
		executePostPasses->Value = 1.0f;
		CCvarManager::RegisterVariable( executePostPasses );
	}


	 void CFrameManager::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		m_hCurrentFrameGraph.Reset();
		m_hCurrentFrameGraph = nullptr;

		MW_INFO( "Shutdown CFrameManager" );
	}


	 void CFrameManager::Render() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( CStaticRenderer::IsMinimized() )
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for( 20ms );
			return;
		}

		RHI::CPipelineManager::BatchCompile();

		// Pre rendering setup
		EResult res = CStaticRenderer::AcquireNextImage( m_CurrentFrameIndex,&m_CurrentImageIndex );
		if ( res == MW_ERROR_SUBOPTIMAL )
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for( 20ms );
			return;
		}
		CStaticRenderer::BeginRootCommandbuffer( m_CurrentFrameIndex );

		auto& presenter = CApplication::GetCreateInfos()->pPresenter;
		auto& currentSwapchainImage = presenter->GetSwapchainImages()[m_CurrentImageIndex];

		currentSwapchainImage->TransitionLayout( m_CurrentFrameIndex, RHI::MW_IMAGE_LAYOUT_GENERAL, RHI::MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, RHI::MW_IMAGE_ASPECT_COLOR_BIT );

		if ( m_hCurrentFrameGraph )
		{

			m_hCurrentFrameGraph->ExecutePreRenderingSteps( m_CurrentFrameIndex );

			//if ( CCvarManager::GetValue( "r_execute_pre_passes" ) != 0.0f )
			m_hCurrentFrameGraph->ExecutePrePasses( m_CurrentFrameIndex );

			m_hCurrentFrameGraph->ExecuteBasePasses( m_CurrentFrameIndex );

			//if ( CCvarManager::GetValue( "r_execute_post_passes" ) != 0.0f )
			m_hCurrentFrameGraph->ExecutePostPasses( m_CurrentFrameIndex );

			m_hCurrentFrameGraph->ExecutePostRenderingSteps( m_CurrentFrameIndex );

		}

		CStaticRenderer::SubmitRootCommandbuffer( m_CurrentFrameIndex );

		CStaticRenderer::Present( m_CurrentFrameIndex );

		m_CurrentFrameIndex = ( m_CurrentFrameIndex + 1 ) % MFIF;
		 
	}



}
