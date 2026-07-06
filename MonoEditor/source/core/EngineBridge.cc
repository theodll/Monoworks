#include "EngineBridge.hh"
#include <QObject>
#include <QQuickWindow>
#include <QTimer>

namespace Monoworks 
{
	void CEngineBridge::SetWindow(QQuickWindow* window) noexcept
	{
		if (!window) return;
		m_QtWindow = window;

		connect(m_QtWindow, &QQuickWindow::beforeRendering, this, &CEngineBridge::OnBeforeRendering, Qt::DirectConnection);

		m_QtWindow->setPersistentGraphics(true);
		m_QtWindow->setColor(Qt::transparent);
		
		QTimer* timer = new QTimer(this);
		timer->setTimerType(Qt::PreciseTimer);
		timer->setInterval(7);


		connect(timer, &QTimer::timeout, this, [this]() {
			if (m_QtWindow)
				m_QtWindow->update();
			});

		timer->start();

	};

	void CEngineBridge::OnBeforeRendering()
	{
		if (m_EngineApplication)
			m_EngineApplication->Frame();
	};
}
