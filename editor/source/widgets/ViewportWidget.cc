#include <Monoworks.hh>

// TODO: make platform dependant
#include <rhi/specific/vulkan/VulkanQtPresenter.hh>
#include <rhi/specific/vulkan/VulkanTexture.hh>

#include "ViewportWidget.hh"

// If anyone bothers or if it becomes a problem, this should probably be changed 
// to be realized with QML like this:
// the texture still gets imported, then gets wrapped into a qml texture. This qml texture then simply is rendered by 
// the QML RHI. But this works right now andf if it works dont fix it.

namespace Monoworks
{
	CViewportWidget::CViewportWidget( RHI::IPresenter* presenter, QWidget* parent ) : QOpenGLWidget( parent )
	{
		MW_PROFILE_FUNC;

		m_pPresenter = presenter;
	};

	CViewportWidget::~CViewportWidget()
	{
		MW_PROFILE_FUNC;

		glDeleteSemaphoresEXT( MFIF, m_RenderFinishedSemaphores );
		glDeleteSemaphoresEXT( MFIF, m_QtReadFinishedSemaphores );

		glDeleteTextures( MFIF, m_PresentationImages );
	}

	void CViewportWidget::Update( u32 imageIndex )
	{
		MW_PROFILE_FUNC;

		m_CurrentImageIndex = imageIndex;
		update();
	};

	void CViewportWidget::paintGL()
	{
		MW_PROFILE_FUNC;

		constexpr GLenum srcEnum = kGlLayoutGeneralEXT;

		glClear( GL_COLOR_BUFFER_BIT );
		
		glWaitSemaphoreEXT(
			m_RenderFinishedSemaphores[m_CurrentImageIndex],
			0,
			nullptr,
			1,
			&m_PresentationImages[m_CurrentImageIndex],
			&srcEnum
		); // fix the opengl errors here

		glUseProgram( m_ShaderProgram );
		glBindTextureUnit( 0, m_PresentationImages[m_CurrentImageIndex] );

		glUniform1i( m_ImageLocation, 0 );

		glBindVertexArray( m_EmptyVAO );
		glDrawArrays( GL_TRIANGLES, 0, 3 );
		glBindVertexArray( 0 );
		
		glSignalSemaphoreEXT(
			m_QtReadFinishedSemaphores[m_CurrentImageIndex],
			0, nullptr, 1,
			&m_PresentationImages[m_CurrentImageIndex],
			&srcEnum
		);
	};

	void CViewportWidget::initializeGL()
	{
		MW_PROFILE_FUNC;

		initializeExternalObjectsFunctions();

#ifndef MW_PLATFORM_OSX
		glEnable( GL_DEBUG_OUTPUT );
		glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS ); 
		glDebugMessageCallback( []( 
			MAYBE_UNUSED GLenum source, 
			MAYBE_UNUSED GLenum type,
			MAYBE_UNUSED GLuint id,
			MAYBE_UNUSED GLenum severity,
			MAYBE_UNUSED GLsizei length,
			MAYBE_UNUSED const GLchar* message, 
			MAYBE_UNUSED const void* userParam )
			{
				if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION )
					return;
				MW_ERROR( "GL Debug [{}]: {}", id, message ); 
			}, nullptr );
#endif
		auto presenter = ( RHI::CVulkanQtPresenter* )m_pPresenter;

		makeCurrent();

		for ( u32 i{}; i < MFIF; i++ )
		{
			// TODO: platform independant
			VmaAllocationInfo2 info{};
			vmaGetAllocationInfo2( *RHI::CVulkanContext::GetAllocator(), *presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetVmaAllocation(), &info );

			// TODO: batch this
			GLuint memory;
			glCreateMemoryObjectsEXT( 1, &memory );
			glGenSemaphoresEXT( 1, &m_RenderFinishedSemaphores[i] );
			glGenSemaphoresEXT( 1, &m_QtReadFinishedSemaphores[i] );

			GLint dedicated = GL_TRUE;
			glMemoryObjectParameterivEXT( memory, kGlDedicatedMemoryObjectEXT, &dedicated );


#ifdef MW_PLATFORM_WINDOWS
			HANDLE imageHandle = presenter->GetPresentationImageWin32Handle( i );
			HANDLE renderSemaphoreHandle = presenter->GetRenderFinishedSemaphoreWin32Handle( i );
			HANDLE readSemaphoreHandle = presenter->GetQtReadFinishedSemaphoreWin32Handle( i );

			glImportMemoryWin32HandleEXT( memory, info.allocationInfo.size, kGlHandleTypeOpaqueWin32EXT, imageHandle );

			glImportSemaphoreWin32HandleEXT( m_RenderFinishedSemaphores[i], kGlHandleTypeOpaqueWin32EXT, renderSemaphoreHandle );
			glImportSemaphoreWin32HandleEXT( m_QtReadFinishedSemaphores[i], kGlHandleTypeOpaqueWin32EXT, readSemaphoreHandle );

#else

			int imageFd = presenter->GetPresentationImageFd( i );
			int renderSemaphoreFd = presenter->GetRenderFinishedSemaphoreFd( i );
			int readSemaphoreFd = presenter->GetQtReadFinishedSemaphoreFd( i ); 

			glImportMemoryFdEXT( memory, info.blockSize, kGlHandleTypeOpaqueFdEXT, imageFd );

			glImportSemaphoreFdEXT( m_RenderFinishedSemaphores[i], kGlHandleTypeOpaqueFdEXT, renderSemaphoreFd );
			glImportSemaphoreFdEXT( m_QtReadFinishedSemaphores[i], kGlHandleTypeOpaqueFdEXT, readSemaphoreFd );
#endif

			glCreateTextures( GL_TEXTURE_2D, 1, &m_PresentationImages[i] );

			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_MAX_LEVEL, 0 );
			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_SWIZZLE_R, GL_BLUE );
			glTextureParameteri( m_PresentationImages[i], GL_TEXTURE_SWIZZLE_B, GL_RED );

			glTextureStorageMem2DEXT(
				m_PresentationImages[i],
				1,
				GL_SRGB8_ALPHA8,
				presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetWidth(),
				presenter->GetSwapchainImages()[i].As<RHI::CVulkanTexture2D>()->GetHeight(),
				memory,
				0
			);

		}

		glGenVertexArrays( 1, &m_EmptyVAO );
		glClearColor( 1, 0, 1, 1 );

		static const char* fullscreenTriangleVertexShader = R"(
		#version 330 core
		
		out vec2 vTexCoord;
		
		void main()
		{
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
		
		uniform sampler2D u_Texture;
		
		void main()
		{
		    FragColor = texture(u_Texture, vTexCoord);
		}
		)";

		GLuint fragmentShader;
		fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

		glShaderSource( fragmentShader, 1, &fullscreenTriangleFragmentShader, NULL );
		glCompileShader( fragmentShader );

		m_ShaderProgram = glCreateProgram();
		glAttachShader( m_ShaderProgram, vertexShader );
		glAttachShader( m_ShaderProgram, fragmentShader );
		glLinkProgram( m_ShaderProgram );

		glDeleteShader( vertexShader );
		glDeleteShader( fragmentShader );

		glUseProgram( m_ShaderProgram );


		m_ImageLocation = glGetUniformLocation( m_ShaderProgram, "u_Texture" );

	};

	void CViewportWidget::resizeGL( MAYBE_UNUSED int w, MAYBE_UNUSED int h )
	{
		MW_PROFILE_FUNC;
	};
}
