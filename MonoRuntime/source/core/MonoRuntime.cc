#include <Monoworks.hh>
#include <core/Application.hh>
#include "../../specific/sdl/EventDispatcher.hh"
#include "MonoRuntime.hh"


int main(int argc, char** argv)
{
	return Monoworks::RuntimeMain(argc, argv);
}

namespace Monoworks 
{
	[[nodiscard]] int RuntimeMain([[maybe_unused]] int argc, [[maybe_unused]] char** argv) 
	{
		CMonoRuntime runtime;

		runtime.Init(argc, argv);

		runtime.Run();

		runtime.Shutdown();

		return 0;
	}

	void CMonoRuntime::Init(int argc, char** argv)
	{
		m_Application = new CApplication;

		CConfigManager cfg("Config/MonoRuntime.cfg");
		cfg.RegisterSection("Runtime");
		cfg.RegisterSection("Rendering");

		cfg.RegisterValue("Runtime", "Title", "MonoEditor");
		cfg.RegisterValue("Runtime", "Window W", "1920");
		cfg.RegisterValue("Runtime", "Window H", "1080");

		cfg.RegisterValue("Rendering", "GAPI", std::format("{}", (int)MW_GAPI_NONE));
		cfg.RegisterValue("Rendering", "Default Width", "1920");
		cfg.RegisterValue("Rendering", "Default Height", "1080");
		cfg.RegisterValue("Rendering", "Resizable", "true");

		cfg.Flush();

		SApplicationCreateInfos appInfos{};
		appInfos.Name = cfg.Get("Runtime", "Title");
		appInfos.RenderableExtent = { cfg.Get<u32>("Rendering", "Default Width"), cfg.Get<u32>("Rendering", "Default Height") };
		appInfos.GraphicsAPI = (EGraphicsAPI)cfg.Get<u32>("Rendering", "GAPI");
		appInfos.ArgumentCount = argc;
		appInfos.Arguments = argv;

		m_Application->Init(&appInfos);

		SWindowCreateInfos windowInfos{};
		windowInfos.GraphicsAPI = (EGraphicsAPI)cfg.Get<u32>("Rendering", "GAPI");;
		windowInfos.WindowTitle = cfg.Get("Runtime", "Title");
		windowInfos.Resizable = cfg.Get<bool>("Rendering", "Resizable");
		windowInfos.WindowExtent = { cfg.Get<u32>("Rendering", "Default Height"), cfg.Get<u32>("Rendering", "Default Width") };

		m_Window = Ref<CWindow>::Create();
		m_Window->Init(&windowInfos);
		m_Dispatcher.Init();
	}

	void CMonoRuntime::Run()
	{
		while(m_Running) 
		{
			m_Dispatcher.ProcessEvents();

			m_Application->Frame();
		}
	}

	void CMonoRuntime::Shutdown()
	{
		m_Dispatcher.Shutdown();
		m_Window->Shutdown();
		m_Application->Shutdown();
	}

}
