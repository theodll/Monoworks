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
		CConfigManager cfg("Config/MonoEditor.cfg");
		cfg.RegisterSection("Editor");
		cfg.RegisterSection("Qt");
		cfg.RegisterSection("Rendering");

		cfg.RegisterValue("Editor", "Title", "MonoEditor");
		cfg.RegisterValue("Editor", "Window W", "1920");
		cfg.RegisterValue("Editor", "Window H", "1080");

		cfg.RegisterValue("Rendering", "GAPI", std::format("{}", (int)MW_GAPI_VULKAN));
		cfg.RegisterValue("Rendering", "Default Width", "1920");
		cfg.RegisterValue("Rendering", "Default Height", "1080");

		cfg.Flush();

		m_QtApplication = new QCoreApplication(argc, argv);

		m_Engine = new CApplication;

		SApplicationCreateInfos appInfos{};
		appInfos.Name = cfg.Get("Editor", "Title");;
		appInfos.RenderableExtent = { cfg.Get<u32>("Rendering", "Default Width"), cfg.Get<u32>("Rendering", "Default Height")};
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
