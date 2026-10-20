#include <mwpch.hh>
#include <core/Application.hh>
#include <events/EventManager.hh>

#include <rhi/specific/vulkan/VulkanRenderer.hh>

#include "StaticRenderer.hh"

namespace Monoworks 
{
        Ref<RHI::IGraphicsAPI> CStaticRenderer::m_pInstance;

		Ref<RHI::ITexture2D> CStaticRenderer::m_hMissingTexture;

		Ref<RHI::ITexture2D> CStaticRenderer::m_hMissingMap;

		Monoworks::FrameGraphicsProfilingData CStaticRenderer::m_GraphicsProfilingData;

		Monoworks::SExtent2D CStaticRenderer::m_RenderableExtent;

		Slang::ComPtr<slang::IGlobalSession>    CStaticRenderer::m_SlangGlobalSession;

        
		 void CStaticRenderer::Init() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			MW_INFO( "Initialize CStaticRenderer" );
			switch ( CApplication::GetGraphicsAPI() )
			{
			case MW_GAPI_NONE: MW_ASSERT( false, "Headless mode not supported" ); break;
			case MW_GAPI_VULKAN: m_pInstance = Ref<RHI::CVulkanRenderer>::Create(); break;
			}

			m_pInstance->Init();
			MW_INFO( "Initialized Rendering Hardware Interface");

			slang::createGlobalSession( m_SlangGlobalSession.writeRef() );

			RHI::CPipelineManager::Init();

			cvar_t missingTexturePath;
			missingTexturePath.Name = "r_missing_texture_path";
			missingTexturePath.String = "resources/MissingTexture.png";
			MW_REG_CVAR( &missingTexturePath );
			
			auto mtstr = CCvarManager::GetString( "r_missing_texture_path" );
			path_t mtpt = "resources/MissingTexture.png";

			m_hMissingTexture = RHI::ITexture2D::Create( &mtpt );

			cvar_t missingMapPath;
			missingMapPath.Name = "r_missing_map_path";
			missingMapPath.String = "resources/MissingMap.png";
			MW_REG_CVAR( &missingMapPath );

			auto mmstr = CCvarManager::GetString( "r_missing_map_path" );
			path_t mmpt = "resources/MissingMap.png";

			m_hMissingMap = RHI::ITexture2D::Create( &mmpt );

			CEventManager::Subscribe( MW_EVENT_APP_FRAME, +[]( SEvent& e )
				{
					MW_PROFILE_FUNC;
					if ( RHI::CPipelineManager::GetTotalCompiledPipelineCount() < RHI::CPipelineManager::GetTotalPipelineCount() )
					{
						RHI::CPipelineManager::BatchCompile();
					}
					return false;
				} );
		}

		void CStaticRenderer::WaitRenderingIdle() NOEXCEPT 
		{
			MW_PROFILE_FUNC;
			return m_pInstance->WaitRenderingIdle();
		};

		 void CStaticRenderer::Shutdown() NOEXCEPT
		{
			MW_PROFILE_FUNC;

			WaitRenderingIdle();
			m_hMissingMap.Reset();
			m_hMissingTexture.Reset();

			RHI::CPipelineManager::Shutdown();
			m_pInstance->Shutdown();
			MW_INFO( "Shutdown CStaticRenderer" );
		}

		 void CStaticRenderer::ProfileFrameData() NOEXCEPT
		{
			MW_PROFILE_FUNC;
		}

		 void CStaticRenderer::BeginRootCommandbuffer( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BeginRootCommandbuffer( frameIndex );
		}

		void CStaticRenderer::SubmitRootCommandbuffer( u32 frameIndex )
		{
			MW_PROFILE_FUNC;
			m_pInstance->SubmitRootCommandbuffer( frameIndex );
		}

		 void CStaticRenderer::BeginSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BeginSecondaryCommandbuffers( frameIndex );
		}

		 void CStaticRenderer::MergeSecondaryCommandbuffers( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->MergeSecondaryCommandbuffers( frameIndex );
		}

		 void CStaticRenderer::DispatchCompute( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, std::span<RHI::DescriptorHandle> ppDesciptors, u32 firstSet, s32 threadID /*= -1*/ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->DispatchCompute( frameIndex, hPipeline, workgroup, ppDesciptors, firstSet, threadID );
		}

		 void CStaticRenderer::DispatchCompute2( u32 frameIndex, Ref<RHI::IComputePipeline> hPipeline, Vector workgroup, s32 threadID /*= -1 */ ) NOEXCEPT
		{
            MW_PROFILE_FUNC;
            return m_pInstance->DispatchCompute2( frameIndex, hPipeline, workgroup, threadID );
		}

		void CStaticRenderer::BeginRendering( u32 frameIndex, const RHI::BeginRenderingInfo* pInfo ) NOEXCEPT
        {
            MW_PROFILE_FUNC;
            return m_pInstance->BeginRendering( frameIndex, pInfo );
        };

        void CStaticRenderer::EndRendering( u32 frameIndex ) NOEXCEPT
        {
            MW_PROFILE_FUNC;
            return m_pInstance->EndRendering( frameIndex );
        };


		 u32 CStaticRenderer::AcquireNextImage( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->AcquireNextImage( frameIndex );
		}

		 void CStaticRenderer::Present( u32 frameIndex ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->Present( frameIndex );
		}

		 void CStaticRenderer::BindGraphicsPipeline( u32 frameIndex, Ref<RHI::IGraphicsPipeline> hPipeline, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BindGraphicsPipeline( frameIndex, hPipeline, threadID );
		}

		 void CStaticRenderer::BindDescriptors( u32 frameIndex, RHI::DescriptorSignature pSignature, std::span<RHI::DescriptorHandle> ppDescriptors, u32 firstSet, s32 threadID /*= -1 */, RHI::EPipelineBindPoint bindpoint /* = RHI::MW_PIPELINE_BIND_POINT_BOTH */ )
		{
			MW_PROFILE_FUNC;
			return m_pInstance->BindDescriptors( frameIndex, pSignature, ppDescriptors, firstSet, threadID, bindpoint );
		}

		 void CStaticRenderer::DrawStaticMeshIndexed( u32 frameIndex, Ref<CMesh> hMesh, const Matrix& transform, s32 threadID, Ref<RHI::IGraphicsPipeline> hPipelineOverride ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->DrawStaticMeshIndexed( frameIndex, hMesh, transform, threadID, hPipelineOverride );
		}

		 void CStaticRenderer::SetDynamicViewports( u32 frameIndex, std::span<const RHI::Viewport> ppViewports, size_t firstViewport ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicViewports( frameIndex, ppViewports, firstViewport );
		}

		 void CStaticRenderer::SetDynamicScissors( u32 frameIndex, std::span<const SExtent2D> ppScissors, size_t firstScissor ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicScissors( frameIndex, ppScissors, firstScissor );
		}

		 void CStaticRenderer::SetDynamicCullMode( u32 frameIndex, RHI::ECullMode cullMode ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicCullMode( frameIndex, cullMode );
		}

		 void CStaticRenderer::SetDynamicViewportsST( u32 frameIndex, std::span<const RHI::Viewport> ppViewports, size_t firstViewport, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicViewportsST( frameIndex, ppViewports, firstViewport, threadID );
		}

		 void CStaticRenderer::SetDynamicScissorsST( u32 frameIndex, std::span<const SExtent2D> pScissors, size_t firstScissor, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicScissorsST( frameIndex, pScissors, firstScissor, threadID );
		}

		 void CStaticRenderer::SetDynamicCullModeST( u32 frameIndex, RHI::ECullMode cullMode, s32 threadID /*= -1 */ ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			return m_pInstance->SetDynamicCullModeST( frameIndex, cullMode, threadID );
		}

}