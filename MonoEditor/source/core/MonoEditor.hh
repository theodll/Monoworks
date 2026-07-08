#include <Monoworks.hh>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

namespace Monoworks 
{
	[[nodiscard]] int EditorMain(int argc, char** argv);

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
