#include <Monoworks.hh>
#include <core/Application.hh>

#include "MonoRuntime.hh"

#include "../../specific/sdl/EventDispatcher.hh"
#include "../rhi/specific/vulkan/VulkanSDLPresenter.hh"


#include <events/EventManager.cc>

#include <tracy/Tracy.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

int main(int argc, char** argv)
{
	return Monoworks::RuntimeMain(argc, argv);
}

namespace Monoworks 
{
	NODISCARD int RuntimeMain([[maybe_unused]] int pArgc, [[maybe_unused]] char** pArgv) 
	{
		CMonoRuntime runtime;

		runtime.Init( pArgc, pArgv );

		runtime.Run();

		runtime.Shutdown();

		return 0;
	}

	void CMonoRuntime::Init( int pArgc, char** pArgv )
	{
		MW_PROFILE_FUNC;
		m_pApplication = new CApplication;

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

		m_pWindow = Ref<CWindow>::Create(&windowInfos);
		m_pPresenter = Ref<RHI::CVulkanSDLPresenter>::Create( windowInfos.WindowExtent, true, ( SDL_Window* )m_pWindow->GetNative() );

		SApplicationCreateInfos appInfos{};
		appInfos.pName = strdup(cfg.Get("Runtime", "Title").c_str());
		appInfos.RenderableExtent = { cfg.Get<u32>("Rendering", "Default Width"), cfg.Get<u32>("Rendering", "Default Height") };
		appInfos.GraphicsAPI = (EGraphicsAPI)cfg.Get<u32>("Rendering", "GAPI");
		appInfos.ArgumentCount = pArgc;
		appInfos.pArguments = pArgv;
		appInfos.Version = { 1, 0, 0 };
		appInfos.RequiredExtensionCallback = +[](u32* extensionCount) { return (const char**)SDL_Vulkan_GetInstanceExtensions(extensionCount); };
		appInfos.pPresenter = m_pPresenter.raw();

		m_pApplication->Init(&appInfos);

		m_Dispatcher.Init();
	}

	void CMonoRuntime::Run()
	{
		CEventManager::Subscribe(MW_EVENT_WINDOW_CLOSE, [this](SEvent& event) { m_Running = false; return true; });

		MW_PROFILE_FUNC;
		while(m_Running) 
		{
			m_Dispatcher.ProcessEvents();

			m_pApplication->Frame();
		}
	}

	void CMonoRuntime::Shutdown()
	{
		MW_PROFILE_FUNC;
		m_Dispatcher.Shutdown();
		m_pWindow->Shutdown();
		m_pApplication->Shutdown();

		delete m_pApplication;
	}

}
