#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
	class CMainWindow;
}
QT_END_NAMESPACE

namespace Monoworks
{
	class CMainWindow : public QMainWindow
	{
		Q_OBJECT
	public:
		CMainWindow(QObject* parent);
		~CMainWindow();

	private:
		Ui::CMainWindow* m_UI;
	};


}
