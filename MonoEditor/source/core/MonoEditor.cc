#include <Monoworks.hh>
#include <core/Application.hh>
#include "MonoEditor.hh"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>


#include <kddockwidgets/MainWindow.h>
#include <kddockwidgets/DockWidget.h>


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

		m_pQtApplication = new QCoreApplication( argc, argv );

		SApplicationCreateInfos appInfos {};
		appInfos.pName = strdup( cfg.Get( "Runtime", "Title" ).c_str() );
		appInfos.RenderableExtent = { cfg.Get<u32>( "Rendering", "Default Width" ), cfg.Get<u32>( "Rendering", "Default Height" ) };
		appInfos.GraphicsAPI = MW_GAPI_VULKAN;
		appInfos.ArgumentCount = argc;
		appInfos.pArguments = argv;
		appInfos.Version = { 1, 0, 0 };
		appInfos.RequiredExtensionCallback = nullptr;
		appInfos.UseQt  = true;

		m_pQtApplication->setOrganizationName( "Monoworks" );
		m_pQtApplication->setApplicationName( "MonoEditor" );

		
		m_pMainWindow = new KDDockWidgets::QtWidgets::MainWindow( appInfos.pName );
		m_pMainWindow->setWindowTitle( appInfos.pName );
		m_pMainWindow->resize( cfg.Get<int>( "Editor", "Window W" ), cfg.Get<int>( "Editor", "Window H" ) );
		m_pMainWindow->show();

		m_pEngineManager = new CEngineManager( &appInfos, m_pMainWindow, m_pQtApplication );
	};

	void CMonoworksEditor::Run()
	{



		int result = m_QtApplication->exec();
	};

	void CMonoworksEditor::Shutdown()
	{
		delete m_MainWindow;
		delete m_Engine;
		delete m_QtApplication;
	};

}
