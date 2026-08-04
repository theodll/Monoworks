#pragma once
#include <Monoworks.hh>

#include <rhi/agnostic/Texture.hh>
#include <rhi/agnostic/Presenter.hh>

#ifdef MW_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace Monoworks::RHI 
{

	class CVulkanQtPresenter : public IPresenter 
	{
	public:
		void Init( const IPresentationInitializationInfo* pInfo ) NOEXCEPT;
		void Init2( const IPresentationInitialization2Info* pInfo ) NOEXCEPT;
		void Shutdown() NOEXCEPT;
		void CreateSurface( const IPresentationSurfaceCreationInfo* pInfo ) NOEXCEPT;
		bool OnResize( SEvent& event );

		NODISCARD u32 Acquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT;
		void TransitionRender( const IPresentationTransitionRenderInfo* pInfo ) NOEXCEPT;
		void TransitionPresent( const IPresentationTransitionPresentInfo* pInfo ) NOEXCEPT;
		void Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT;

#ifdef MW_PLATFORM_WINDOWS
		HANDLE GetPresentationImageWin32Handle( u32 imageIndex );
		HANDLE GetRenderFinishedSemaphoreWin32Handle( u32 imageIndex ) { return m_RenderFinishedSemaphoreWin32Handles[imageIndex]; };
#else 
		int GetPresentationImageFd( u32 imageIndex ) { return (m_RenderFinishedSemaphoreFds[imageIndex] >= 0) ? return m_RenderFinishedSemaphoreFds[imageIndex] : MW_ASSERT( "Requested Presentation Image File Descriptor is invalid." ); };
		int GetRenderFinishedSemaphoreFd( u32 imageIndex ) { return (m_RenderFinishedSemaphoreFds[imageIndex] >= 0) ? return m_RenderFinishedSemaphoreFds[imageIndex] : MW_ASSERT( "Requested Render Finished Semaphore File Descriptor is invalid." ); };
#endif


		// TODO: rename this
		NODISCARD std::vector<Ref<ITexture2D>>& GetSwapchainImages() NOEXCEPT { return m_PresentationImages; };
		NODISCARD void* GetSurface() NOEXCEPT { MW_API_ERROR("Invalid Surface access, There is no surface."); return nullptr; };
	private:
		std::vector<Ref<ITexture2D>> m_PresentationImages;

#ifdef MW_PLATFORM_WINDOWS
		HANDLE m_PresentationImageWin32Handles[MFIF];
		HANDLE m_RenderFinishedSemaphoreWin32Handles[MFIF];
#else
		int m_PresentationImageWin32Handles[MFIF] = { -1 };
		int m_RenderFinishedSemaphoreFds[MFIF] = { -1 };
#endif
	};

}