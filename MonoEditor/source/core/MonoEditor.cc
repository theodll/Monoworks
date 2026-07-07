#include <Monoworks.hh>
#include <core/Application.hh>
#include "MonoEditor.hh"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

int main(int argc, char** argv)
{
	return Monoworks::EditorMain(argc, argv);
}

namespace Monoworks 
{
	[[nodiscard]] int EditorMain(int argc, char** argv) 
	{
		CMonoworksEditor editor;

		editor.Init(argc, argv);

		editor.Run();

		editor.Shutdown();

		return 0;
	}

	void CMonoworksEditor::Init(int argc, char** argv)
	{
		m_QtApplication = new QCoreApplication(argc, argv);

		m_Engine = new CApplication;

		SApplicationCreateInfos appInfos{};
		appInfos.Name = "MonoEditor";
		appInfos.RenderableExtent = { 670, 480 };
		appInfos.ArgumentCount = argc;
		appInfos.Arguments = argv;


		m_Engine->Init(&appInfos);
	};

	void CMonoworksEditor::Run()
	{



		int result = m_QtApplication->exec();
	};

	void CMonoworksEditor::Shutdown()
	{
		delete m_Engine;
		delete m_QtApplication;
	};

}
