#include <Monoworks.hh>

#include "ViewportWidget.h"

namespace Monoworks 
{
	CViewportWidget::CViewportWidget( RHI::IPresenter* presenter, QWidget* parent ) : QOpenGLWidget( parent )
	{
		MW_PROFILE_FUNC;

		m_pPresenter = presenter;
	};

	void CViewportWidget::initializeGL() 
	{
		MW_PROFILE_FUNC;
	};

	void CViewportWidget::paintGL() 
	{
		MW_PROFILE_FUNC;
	};

	void CViewportWidget::resizeGL( int w, int h ) 
	{
		MW_PROFILE_FUNC;
	};
}
