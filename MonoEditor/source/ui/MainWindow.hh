/**
 * @file MainWindow.hh
 * @author Theo Wimber (theowimber@abeams.app) 
 * @brief Qt Main Window Class
 * @version 0.1
 * @date 2026-07-08
 * @ingroup Editor
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
	class CMainWindow;
}
QT_END_NAMESPACE

namespace Monoworks
{
	/**
	 * @brief Qt Main Window Class for the Editor
	 */
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
