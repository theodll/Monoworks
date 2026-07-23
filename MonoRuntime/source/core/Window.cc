#include "Window.hh"
#include <common/Log.h>

namespace Monoworks
{
	CWindow::CWindow(const SWindowCreateInfos* pInfos) noexcept
	{
		MW_PROFILE_FUNC;
		m_WindowExtent = pInfos->WindowExtent;

		const char* title = pInfos->WindowTitle.c_str();

		u32 flags = pInfos->GraphicsAPI;

		if (pInfos->Resizable)
			flags = flags | SDL_WINDOW_RESIZABLE;

		m_SDLWindow = SDL_CreateWindow(title, pInfos->WindowExtent.Width, pInfos->WindowExtent.Height, flags);

		MW_INFO("Initialize CWindow");
	}

	void CWindow::Shutdown() noexcept
	{
		MW_PROFILE_FUNC;
		SDL_DestroyWindow(m_SDLWindow);
		MW_INFO("Shutdown CWindow");
	}
}	


