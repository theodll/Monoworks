#pragma once
#include <Monoworks.hh>

#include <QOpenGLWidget>

namespace Monoworks
{
	// plan for the viewport & presenter is: 
	// the presenter manages the "swapchain" images and hands out the memory descriptors & handles to the
	// Viewport Widget (this class). Then OpenGL imports the memory descriptor for the image and the render finished semaphore
	class CViewportWidget : public QOpenGLWidget
	{
		Q_OBJECT;
	public:
		CViewportWidget( RHI::IPresenter* presenter, QWidget* parent = nullptr );

		virtual void initializeGL() override;
		virtual void paintGL() override;
		virtual void resizeGL( int w, int h ) override;
	private:

		RHI::IPresenter* m_pPresenter;

	};
}