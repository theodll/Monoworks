#include <Monoworks.hh>
#include <core/Application.hh>
#include "../../specific/sdl/EventDispatcher.hh"
#include "MonoRuntime.hh"
#include <events/EventManager.cc>

#include <Tracy/Tracy.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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
		MW_PROFILE_FUNC;
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

		SWindowCreateInfos windowInfos{};
		windowInfos.GraphicsAPI = (EGraphicsAPI)cfg.Get<u32>("Rendering", "GAPI");;
		windowInfos.WindowTitle = cfg.Get("Runtime", "Title");
		windowInfos.Resizable = cfg.Get<bool>("Rendering", "Resizable");
		windowInfos.WindowExtent = { cfg.Get<u32>("Rendering", "Default Height"), cfg.Get<u32>("Rendering", "Default Width") };

		m_Window = Ref<CWindow>::Create(&windowInfos);

		SApplicationCreateInfos appInfos{};
		appInfos.Name = strdup(cfg.Get("Runtime", "Title").c_str());
		appInfos.RenderableExtent = { cfg.Get<u32>("Rendering", "Default Width"), cfg.Get<u32>("Rendering", "Default Height") };
		appInfos.GraphicsAPI = (EGraphicsAPI)cfg.Get<u32>("Rendering", "GAPI");
		appInfos.ArgumentCount = argc;
		appInfos.Arguments = argv;
		appInfos.Version = { 1, 0, 0 };
		appInfos.RequiredExtensionCallback = +[](u32* extensionCount) { return (const char**)SDL_Vulkan_GetInstanceExtensions(extensionCount); };

		m_Application->Init(&appInfos);

		m_Dispatcher.Init();
	}

	void CMonoRuntime::Run()
	{
		CEventManager::Subscribe(MW_EVENT_WINDOW_CLOSE, [this](SEvent& event) { m_Running = false; return true; });

		MW_PROFILE_FUNC;
		while(m_Running) 
		{
			m_Dispatcher.ProcessEvents();

			m_Application->Frame();
		}
	}

	void CMonoRuntime::Shutdown()
	{
		MW_PROFILE_FUNC;
		m_Dispatcher.Shutdown();
		m_Window->Shutdown();
		m_Application->Shutdown();

		delete m_Application;
	}

}
