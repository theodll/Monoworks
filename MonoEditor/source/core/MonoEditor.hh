/**
 * @defgroup Editor Editor
 * @brief Everything in the Editor Project
 */

/**
 * @file MonoEditor.hh
 * @author Theo Wimber (theowimber@abeams.app) 
 * @brief MonoEditor's root file
 * @version 0.1
 * @date 2026-07-08
 * @ingroup Editor
 * @copyright Copyright (c) 2026
 * 
 */

#include <Monoworks.hh>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

namespace Monoworks 
{
	/**
	 * @brief Generic main function of the runtime.
	 * For example: main, WinMain, etc.
	 * @param argc Number of arguments in the array of arguments
	 * @param argv Array of arguments
	 * @return int Return code.
	 */
	[[nodiscard]] int EditorMain(int argc, char** argv);

	/**
	 * @brief Class to house the MonoEditor
	 */
	class CMonoworksEditor
	{
	public:
		CMonoworksEditor() = default;

		void Init(int argc, char** argv);
		void Run();
		void Shutdown();

	private:
		CApplication* m_Engine;
		QCoreApplication* m_QtApplication;
	};
	
}
