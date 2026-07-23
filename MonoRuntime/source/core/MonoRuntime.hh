/**
 * @defgroup Runtime Runtime
 * @brief Everything in the Runtime Project
 */

/**
 * @file MonoRuntime.hh
 * @author Theo Wimber (theowimber@abeams.app) 
 * @brief MonoRuntimes's root file
 * @version 0.1
 * @date 2026-07-08
 * @ingroup Runtime
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include <Monoworks.hh>

#include <rhi/agnostic/Presenter.hh>
#include "../../specific/sdl/EventDispatcher.hh"

#include "Window.hh"


namespace Monoworks 
{
	/**
	 * @brief Generic main function of the runtime.
	 * For example: main, WinMain, etc.
	 * @param argc Number of arguments in the array of arguments
	 * @param argv Array of arguments
	 * @return int Return code.
	 */
	[[nodiscard]] int RuntimeMain(int pArgc, char** pArgv);

	/**
	 * @brief Class to house the MonoRuntime
	 */
	class CMonoRuntime 
	{
	public:
		CMonoRuntime() = default;

		/**
		 * @brief Initializes the runtime 
		 * @param argc Number of arguments in the array of arguments
		 * @param argv Array of arguments
		 */
		void Init(int pArgc, char** pArgv);

		/**
		 * @brief Main Loop of the runtime
		 */
		void Run();

		/**
		 * @brief Gently shuts down the runtime 
		 */
		void Shutdown();
	
	private:
		CApplication* m_pApplication;
		Ref<CWindow> m_pWindow; 
		Ref<RHI::IPresenter> m_pPresenter;
		CSDLEventDispatcher m_Dispatcher;

		bool m_Running;
	};

}
