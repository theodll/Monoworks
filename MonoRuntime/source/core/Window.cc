#include "Window.hh"
#include <common/Log.h>

namespace Monoworks
{


	void CWindow::Init(const SWindowCreateInfos* pInfos) noexcept
	{
		m_WindowExtent = pInfos->WindowExtent;

		const char* title = pInfos->WindowTitle.c_str();
		
		u32 flags = pInfos->GraphicsAPI;
		
		if (pInfos->Resizable)
			flags = flags | SDL_WINDOW_RESIZABLE;

		m_SDLWindow = SDL_CreateWindow(title, pInfos->WindowExtent.Width, pInfos->WindowExtent.Height, flags);
		
		MW_INFO("Created a SDL Window");
	}

	void CWindow::Shutdown() noexcept
	{
		SDL_DestroyWindow(m_SDLWindow);
	}
}


