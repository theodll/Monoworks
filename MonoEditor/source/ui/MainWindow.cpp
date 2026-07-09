#include <QMainWindow>

#include "MainWindow.hh"
#include "ui_MainWindow.h"
namespace Monoworks
{

	CMainWindow::CMainWindow(QObject* parent)
		: QMainWindow(), m_UI(new ::Ui::CMainWindow)
	{
		m_UI->setupUi(this);
	}

	CMainWindow::~CMainWindow()
	{
	}

}