#include <Monoworks.hh>
#include <core/Application.hh>
#include "MonoEditor.hh"

#include <QApplication>
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
	[[nodiscard]] int EditorMain(int argc, char** pArgv) 
	{
		CMonoworksEditor editor;

		try
		{
			editor.Init( argc, pArgv );
		}
		catch ( const CFatalException& e )
		{
			MW_API_ERROR( "Unhandled editor initialization fatal exception: {}.", e.what() );
			MW_DEBUG_BREAK;
			std::terminate();
		}
		catch ( const std::exception& e )
		{
			MW_API_ERROR( "Unhandled editor intialization exception: {}.", e.what() );
		}
		catch ( ... )
		{
			MW_API_WARN( "Unhandled editor initialization throw." );
		}

		try
		{
			editor.Run();
		}
		catch ( const CFatalException& e )
		{
			MW_API_ERROR( "Unhandled editor frame fatal exception: {}.", e.what() );
			MW_DEBUG_BREAK;
			std::terminate();
		}
		catch ( const std::exception& e )
		{
			MW_API_ERROR( "Unhandled editor frame exception: {}.", e.what() );
		}
		catch ( ... )
		{
			MW_API_WARN( "Unhandled editor frame throw." );
		}

		try
		{
			editor.Shutdown();
		}
		catch ( const CFatalException& e )
		{
			MW_API_ERROR( "Unhandled editor shutdown fatal exception: {}.", e.what() );
			MW_DEBUG_BREAK;
			std::terminate();
		}
		catch ( const std::exception& e )
		{
			MW_API_ERROR( "Unhandled editor shutdown exception: {}.", e.what() );
		}
		catch ( ... )
		{
			MW_API_WARN( "Unhandled editor shutdown throw." );
		}

		return 0;
	}

	void CMonoworksEditor::Init(int argc, char** argv)
	{
		MW_PROFILE_FUNC;
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

		QApplication::setAttribute( Qt::AA_UseDesktopOpenGL );

		m_pQtApplication = new QApplication( argc, argv );

		SApplicationCreateInfos appInfos {};
#ifdef MW_PLATFORM_WINDOWS
		appInfos.pName = _strdup( cfg.Get( "Editor", "Title" ).c_str() );
#else
		appInfos.pName = strdup( cfg.Get( "Editor", "Title" ).c_str() );
#endif
		appInfos.RenderableExtent = { cfg.Get<u32>( "Rendering", "Default Width" ), cfg.Get<u32>( "Rendering", "Default Height" ) };
		appInfos.GraphicsAPI = MW_GAPI_VULKAN;
		appInfos.ArgumentCount = argc;
		appInfos.pArguments = argv;
		appInfos.Version = { 1, 0, 0 };
		appInfos.RequiredExtensionCallback = nullptr;
		appInfos.UseQt  = true;

		m_pQtApplication->setOrganizationName( "Monoworks" );
		m_pQtApplication->setApplicationName( "MonoEditor" );
		
		KDDockWidgets::initFrontend( KDDockWidgets::FrontendType::QtWidgets );

		KDDockWidgets::MainWindowOptions options = KDDockWidgets::MainWindowOption_HasCentralGroup;
		m_pMainWindow = new KDDockWidgets::QtWidgets::MainWindow( appInfos.pName, options );

		m_pMainWindow->setWindowTitle( appInfos.pName );
		m_pMainWindow->resize( cfg.Get<int>( "Editor", "Window W" ), cfg.Get<int>( "Editor", "Window H" ) );
		m_pMainWindow->show();

		m_pEngineManager = new CEngineManager( &appInfos, m_pMainWindow, m_pQtApplication );
	};

	void CMonoworksEditor::Run()
	{
		MW_PROFILE_FUNC;
		m_pQtApplication->exec();
	};

	void CMonoworksEditor::Shutdown()
	{
		MW_PROFILE_FUNC;
		delete m_pMainWindow;
		delete m_pEngineManager;
		delete m_pQtApplication;
	};

}
