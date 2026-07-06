#pragma once
#include <Monoworks.hh>
#include <core/Application.hh>

#include <QObject>
#include <QQuickWindow>

namespace Monoworks
{

	class CEngineBridge : public QObject
	{
		Q_OBJECT
	public:
		explicit CEngineBridge(CApplication* app) noexcept : m_EngineApplication(app) {};

		void SetWindow(QQuickWindow* window) noexcept;

	public slots:
		void OnBeforeRendering();

	private:
		CApplication* m_EngineApplication = nullptr;
		QQuickWindow* m_QtWindow = nullptr;

	}

}
