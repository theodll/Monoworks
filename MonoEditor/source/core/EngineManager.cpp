#include <Monoworks.hh>

// TODO: make agnostic
#include <rhi/specific/vulkan/VulkanQtPresenter.hh>

#include <renderer/StaticRenderer.hh>

#include "EngineManager.h"

#include <QTimer>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

namespace Monoworks 
{
	CEngineManager::CEngineManager( SApplicationCreateInfos* pCreateInfos, KDDockWidgets::QtWidgets::MainWindow* pMainWindow, QObject* pParent ) : QObject(pParent)
	{
		MW_PROFILE_FUNC;

		m_pEngine = new CApplication();

		m_pPresenter = new RHI::CVulkanQtPresenter( pCreateInfos->RenderableExtent );
		pCreateInfos->pPresenter = m_pPresenter;

		m_pEngine->Init( pCreateInfos );

		// TODO: GAPI Agnostig
		m_ViewportCount++;
		m_pViewports.push_back( new CViewportWidget(m_pPresenter, pMainWindow ) );
		

		for ( u32 i {}; i < m_ViewportCount; i++ )
		{
			auto dock = new KDDockWidgets::QtWidgets::DockWidget( QString::fromStdString( std::format( "Viewport {}", i ) ) );
			dock->setWidget( m_pViewports[i] );
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


		for ( u32 i{}; i < m_ViewportCount; i++ )
		{
			m_pViewports[i]->Update( CStaticRenderer::GetCurrentImageIndex() );
		}
		

	};
}