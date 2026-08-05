#pragma once
#include <Monoworks.hh>

#include <widgets/ViewportWidget.h>
#include <rhi/agnostic/Presenter.hh>

#include <kddockwidgets/MainWindow.h>s
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
		CEngineManager( SApplicationCreateInfos* createInfos, KDDockWidgets::QtWidgets::MainWindow* mainWindow, QObject* pParent = nullptr );
		~CEngineManager();
		
		void Tick();

		CViewportWidget** GetViewports() { return m_pViewports; }
		size_t GetViewportCount() { return m_ViewportCount; }

	private:
		CApplication* m_pEngine;
		KDDockWidgets::QtWidgets::MainWindow* m_pMainWindow;

		RHI::IPresenter* m_pPresenter;

		CViewportWidget** m_pViewports;
		size_t m_ViewportCount;
	};
}