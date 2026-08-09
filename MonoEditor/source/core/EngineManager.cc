#include <Monoworks.hh>

// TODO: make agnostic
#include <rhi/specific/vulkan/VulkanQtPresenter.hh>
#include <renderer/StaticRenderer.hh>

#include "EngineManager.hh"

#include <QTimer>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

namespace Monoworks 
{
	CEngineManager::CEngineManager( SApplicationCreateInfos* pCreateInfos, KDDockWidgets::QtWidgets::MainWindow* pMainWindow, QObject* pParent ) : QObject(pParent)
	{
		MW_PROFILE_FUNC;

		m_pEngine = new CApplication();

		// TODO: api agnostic
		m_pPresenter = new RHI::CVulkanQtPresenter( pCreateInfos->RenderableExtent );
		pCreateInfos->pPresenter = m_pPresenter;

		m_pEngine->Init( pCreateInfos );

		m_Viewports.push_back( new CViewportWidget(m_pPresenter, pMainWindow ) );
		
		for ( size_t i {}; i < m_Viewports.size(); i++ )
		{
			std::string viewportName = ( i > 0 ) ? "Viewport" : std::format( "Viewport {}", i);
			auto dock = new KDDockWidgets::QtWidgets::DockWidget( QString::fromStdString( viewportName ) );
			dock->setWidget( m_Viewports[i] );
			pMainWindow->addDockWidgetAsTab( dock );
		}

		QTimer* timer = new QTimer( this );
		connect( timer, &QTimer::timeout, this, &CEngineManager::Tick );
		timer->start();
	}

	CEngineManager::~CEngineManager()
	{
		MW_PROFILE_FUNC;

		// m_pPresenter->Shutdown();

//		m_pEngine->Shutdown();
	}

	void CEngineManager::Tick() 
	{
		MW_PROFILE_FUNC;

		m_pEngine->Frame();


		for ( auto& viewport : m_Viewports )
		{
			viewport->Update( CStaticRenderer::GetCurrentImageIndex() );
		}
		

	};
}