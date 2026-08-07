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

	CViewportWidget::~CViewportWidget()
	{

	}

	void CViewportWidget::Update( u32 imageIndex )
	{
		MW_PROFILE_FUNC;

		constexpr GLenum srcEnum = kGlLayoutShaderReadOnlyEXT;
		
		glClear(GL_COLOR_BUFFER_BIT),

		glWaitSemaphoreEXT(
			m_RenderFinishedSemaphores[imageIndex],
			0,
			nullptr,
			1,
			&m_PresentationImages[imageIndex],
			&srcEnum
		);

		glBindTextureUnit( 0, m_PresentationImages[imageIndex] );

		glUniform1i( m_ImageLocation, 0 );

		glBindVertexArray( m_EmptyVAO );
		glDrawArrays( GL_TRIANGLES, 0, 3 );
		glBindVertexArray( 0 );
	};

	void CViewportWidget::initializeGL() 
	{
		MW_PROFILE_FUNC;

		initializeExternalObjectsFunctions();

		auto presenter = ( RHI::CVulkanQtPresenter* )m_pPresenter; 

		makeCurrent();

		for ( u32 i{}; i < MFIF; i++ )
		{
			// TODO: platform independant
			VmaAllocationInfo2 info {};
			vmaGetAllocationInfo2( *RHI::CVulkanContext::GetAllocator(), *presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetVmaAllocation(), &info );

			// TODO: batch this
			GLuint memory;
			glCreateMemoryObjectsEXT( 1, &memory );

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
			auto image = m_PresentationImages[i];
			
			glCreateTextures( GL_TEXTURE_2D, 1, &image );

			glTextureParameteri( image, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTextureParameteri( image, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTextureParameteri( image, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTextureParameteri( image, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

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

		glGenVertexArrays(1, &m_EmptyVAO);
		glClearColor( 1, 0, 0, 1 );

		m_ImageLocation = glGetUniformLocation( m_ShaderProgram, "u_Texture" );

		static const char* fullscreenTriangleVertexShader = R"(
		#version 330 core
		
		out vec2 vTexCoord;
		
		void main()
		{
		    // Generiert ein Fullscreen-Dreieck aus 3 Vertices ohne Buffer
		    // Vertex 0: (-1, -1), UV: (0, 0)
		    // Vertex 1: ( 3, -1), UV: (2, 0)
		    // Vertex 2: (-1,  3), UV: (0, 2)
		    float x = -1.0 + float((gl_VertexID & 1) << 2);
		    float y = -1.0 + float((gl_VertexID & 2) << 1);
		
		    vTexCoord = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
		    gl_Position = vec4(x, y, 0.0, 1.0);
		}
		)";

		GLuint vertexShader;
		vertexShader = glCreateShader( GL_VERTEX_SHADER );

		glShaderSource( vertexShader, 1, &fullscreenTriangleVertexShader, NULL );
		glCompileShader( vertexShader );

		static const char* fullscreenTriangleFragmentShader = R"(
		#version 330 core
		
		in vec2 vTexCoord;
		out vec4 FragColor;
		
		uniform sampler2D uTexture;
		
		void main()
		{
		    FragColor = texture(uTexture, vTexCoord);
		}
		)";

		GLuint fragmentShader;
		fragmentShader = glCreateShader( GL_VERTEX_SHADER );

		glShaderSource( fragmentShader, 1, &fullscreenTriangleFragmentShader, NULL );
		glCompileShader( fragmentShader );

		m_ShaderProgram = glCreateProgram();
		glAttachShader( m_ShaderProgram, vertexShader );
		glAttachShader( m_ShaderProgram, fragmentShader );
		glLinkProgram( m_ShaderProgram );

		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );

		glUseProgram( m_ShaderProgram );

	};

	void CViewportWidget::resizeGL( int w, int h ) 
	{
		MW_PROFILE_FUNC;
	};
}
