#include <Monoworks.hh>

// todo make platform dependant

#include <rhi/specific/vulkan/VulkanQtPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>

#include "ViewportWidget.h"

namespace Monoworks 
{
	CViewportWidget::CViewportWidget( RHI::IPresenter* presenter, QWidget* parent ) : QOpenGLWidget( parent )
	{
		MW_PROFILE_FUNC;

		m_pPresenter = presenter;
	};

	void CViewportWidget::Update()
	{
		MW_PROFILE_FUNC;
	};

	void CViewportWidget::initializeGL() 
	{
		MW_PROFILE_FUNC;

		initializeExternalObjectsFunctions();

		auto presenter = ( RHI::CVulkanQtPresenter* )m_pPresenter; 

		for ( u32 i{}; i < MFIF; i++ )
		{
			VmaAllocationInfo2 info {};
			vmaGetAllocationInfo2( *RHI::CVulkanContext::GetAllocator(), *presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetVmaAllocation(), &info );

			GLuint memory;

#ifdef MW_PLATFORM_WINDOWS
			HANDLE imageHandle = presenter->GetPresentationImageWin32Handle( i );
			HANDLE semaphoreHandle = presenter->GetRenderFinishedSemaphoreWin32Handle( i );

			glImportSemaphoreWin32HandleEXT( m_RenderFinishedSemaphores[i], kGlHandleTypeOpaqueWin32EXT, semaphoreHandle );
			glImportMemoryWin32HandleEXT( memory, info.blockSize, kGlHandleTypeOpaqueWin32EXT, imageHandle );

#else

			int imageFd = presenter->GetPresentationImageFd( i );
			int semaphoreFd = presenter->GetRenderFinishedSemaphoreFd( i );

			glImportMemoryFdEXT( memory, info.blockSize, kGlHandleTypeOpaqueFdEXT, imageFd );
			glImportSemaphoreFdEXT( m_RenderFinishedSemaphores[i], kGlHandleTypeOpaqueFdEXT, semaphoreFd );
#endif
			glCreateTextures( GL_TEXTURE_2D, 1, &m_PresentationImages[i] );

			glTextureStorageMem2DEXT(
				m_PresentationImages[i],
				1,
				GL_BGRA,
				presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetWidth(),
				presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetHeight(),
				memory,
				0
			);

		}

		// create shaders
		
	};

	void CViewportWidget::resizeGL( int w, int h ) 
	{
		MW_PROFILE_FUNC;
	};
}
