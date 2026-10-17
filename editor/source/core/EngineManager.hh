#pragma once
#include <Monoworks.hh>

#include <widgets/ViewportWidget.hh>
#include <rhi/agnostic/Presenter.hh>

#include <kddockwidgets/MainWindow.h>
#include <QWidget>

namespace Monoworks 
{
	/*
	* @brief Manages the Engine Application
	*/
	class CEngineManager : public QObject 
	{
		Q_OBJECT;
	public:
		CEngineManager( SApplicationCreateInfos* pCreateInfos, KDDockWidgets::QtWidgets::MainWindow* pMainWindow, QObject* pParent = nullptr );
		~CEngineManager();
		
		void Tick();

		CViewportWidget** GetViewports() { return m_Viewports.data(); }
		size_t GetViewportCount() { return m_Viewports.size(); }

	private:
		CApplication* m_pEngine;
		KDDockWidgets::QtWidgets::MainWindow* m_pMainWindow;

		RHI::IPresenter* m_pPresenter;

		std::vector<CViewportWidget*> m_Viewports;
	};
}