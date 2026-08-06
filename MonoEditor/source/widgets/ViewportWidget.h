#pragma once
#include <Monoworks.hh>
#include <common/QOpenGLExternalObjectsExtraFunctions.hh>

#include <QOpenGLWidget>

namespace Monoworks
{
	// plan for the viewport & presenter is: 
	// the presenter manages the "swapchain" images and hands out the memory descriptors & handles to the
	// Viewport Widget (this class). Then OpenGL imports the memory descriptor for the image and the render finished semaphore
	class CViewportWidget : public QOpenGLWidget, protected QOpenGLExternalObjectsExtraFunctions
	{
		Q_OBJECT;
	public:
		CViewportWidget( RHI::IPresenter* presenter, QWidget* parent = nullptr );

		void Update();

		virtual void initializeGL() override;
		virtual void resizeGL( int w, int h ) override;
	private:
		RHI::IPresenter* m_pPresenter;

		GLuint m_PresentationImages[MFIF];
		GLuint m_RenderFinishedSemaphores[MFIF];
		GLuint m_ShaderProgram;


	};
}