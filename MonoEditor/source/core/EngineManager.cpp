#include <Monoworks.hh>

#include <rhi/specific/vulkan/VulkanQtPresenter.hh>

#include "EngineManager.h"

namespace Monoworks 
{
	CEngineManager::CEngineManager( SApplicationCreateInfos* createInfos, KDDockWidgets::QtWidgets::MainWindow* mainWindow, QObject* pParent ) : QObject(pParent)
	{
		MW_PROFILE_FUNC;

		m_pEngine = new CApplication();
		m_pEngine->Init( createInfos ); 

		// TODO: GAPI Agnostic
		m_pPresenter = new RHI::CVulkanQtPresenter;
		


		m_pViewports[0] = new CViewportWidget(m_pPresenter, mainWindow);
	}

	CEngineManager::~CEngineManager()
	{

	}

	void CEngineManager::Tick() 
	{
		MW_PROFILE_FUNC;
	};
}