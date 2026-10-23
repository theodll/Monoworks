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
		try 
		{
			runtime.Init( pArgc, pArgv );
		}
		catch ( const CFatalException& e)
		{
			MW_API_ERROR( "Unhandled runtime initialization fatal exception: {}.", e.what() );
			MW_DEBUG_BREAK;
			return e.GetCode();
		}
		catch ( const std::exception& e )
		{
			MW_API_ERROR( "Unhandled runtime intialization exception: {}.", e.what() );
			return MW_ERROR_UNKNOWN;
		}
		catch ( ... )
		{
			MW_API_WARN( "Unhandled runtime initialization throw." );
			return MW_ERROR_UNKNOWN;
		}

		try 
		{
			runtime.Run();
		}
		catch ( const CFatalException& e )
		{
			MW_API_ERROR( "Unhandled runtime frame fatal exception: {}.", e.what() );
			MW_DEBUG_BREAK;
			return e.GetCode();
		}
		catch ( const std::exception& e )
		{
			MW_API_ERROR("Unhandled runtime frame exception: {}.", e.what() );
			return MW_ERROR_UNKNOWN;
		}
		catch ( ... )
		{
			MW_API_WARN( "Unhandled runtime frame throw." );
			return MW_ERROR_UNKNOWN;
		}
	
		try
		{
			runtime.Shutdown();
		} 
		catch ( const CFatalException& e )
		{
			MW_API_ERROR( "Unhandled runtime shutdown fatal exception: {}.", e.what() );
			MW_DEBUG_BREAK;
			return e.GetCode();
		}
		catch ( const std::exception& e )
		{
			MW_API_ERROR( "Unhandled runtime shutdown exception: {}.", e.what() );
			return MW_ERROR_UNKNOWN;
		}
		catch ( ... )
		{
			MW_API_WARN( "Unhandled runtime shutdown throw." );
			return MW_ERROR_UNKNOWN;
		}


		return 0;
	}

	void CMonoRuntime::Init( int pArgc, char** pArgv )
	{
		MW_PROFILE_FUNC;
		m_pApplication = new CApplication;

		CConfigManager cfg("Config/MonoRuntime.cfg");
		cfg.RegisterSection("Runtime");
		cfg.RegisterSection("Rendering");

		cfg.RegisterValue("Runtime", "Title", "Mono Runtime");
		cfg.RegisterValue("Runtime", "Window W", "1920");
		cfg.RegisterValue("Runtime", "Window H", "1080");

		cfg.RegisterValue("Rendering", "GAPI", std::format("{}", (int)MW_GAPI_VULKAN));
		cfg.RegisterValue("Rendering", "Default Width", "1920");
		cfg.RegisterValue("Rendering", "Default Height", "1080");
		cfg.RegisterValue("Rendering", "Resizable", "true");

		cfg.Flush();

		SWindowCreateInfos windowInfos{};
		windowInfos.GraphicsAPI = MW_GAPI_VULKAN;
		windowInfos.WindowTitle = cfg.Get("Runtime", "Title");
		windowInfos.Resizable = cfg.Get<bool>("Rendering", "Resizable");
		windowInfos.WindowExtent = { cfg.Get<u32>("Rendering", "Default Width"), cfg.Get<u32>("Rendering", "Default Height") };

		m_pWindow = Ref<CWindow>::Create(&windowInfos);
		m_pPresenter = Ref<RHI::CVulkanSDLPresenter>::Create( windowInfos.WindowExtent, true, ( SDL_Window* )m_pWindow->GetNative() );

		SApplicationCreateInfos appInfos{};
		// fight /WX & /W4
#ifdef MW_PLATFORM_WINDOWS
		appInfos.pName = _strdup(cfg.Get("Runtime", "Title").c_str());
#else
		appInfos.pName = strdup( cfg.Get( "Runtime", "Title" ).c_str() );
#endif
		appInfos.RenderableExtent = { cfg.Get<u32>("Rendering", "Default Width"), cfg.Get<u32>("Rendering", "Default Height") };
		appInfos.GraphicsAPI = MW_GAPI_VULKAN;
		appInfos.ArgumentCount = pArgc;
		appInfos.pArguments = pArgv;
		appInfos.Version = { 1, 0, 0 };
		appInfos.RequiredExtensionCallback = +[](u32* extensionCount) { return (const char**)SDL_Vulkan_GetInstanceExtensions(extensionCount); };
		appInfos.pPresenter = m_pPresenter.raw();
		appInfos.UseSDL = true;
		appInfos.UseSwapchain = true;

		m_pApplication->Init(&appInfos);

		m_Dispatcher.Init();
	}

	void CMonoRuntime::Run()
	{
		MW_PROFILE_FUNC;
		static const double invFreq = 1.0 / ( double )SDL_GetPerformanceFrequency();
		static float lastFrameTime = 0;
		
		u64 counter = SDL_GetPerformanceCounter();
		double time = ( double )counter * invFreq;

		CTimestep ts = time - lastFrameTime;
		lastFrameTime = time;

		CEventManager::Subscribe( MW_EVENT_WINDOW_CLOSE, [this]( MAYBE_UNUSED SEvent& event ) { m_Running = false; return true; } );
		while(m_Running) 
		{
			m_Dispatcher.ProcessEvents();

			m_pApplication->Frame( ts );
		}
	}

	void CMonoRuntime::Shutdown()
	{
		MW_PROFILE_FUNC;
		m_Dispatcher.Shutdown();
		m_pWindow->Shutdown();
		m_pApplication->Shutdown();

		free( ( void* )m_pApplication->GetCreateInfos()->pName );

		delete m_pApplication;
	}

}
