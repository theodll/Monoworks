#pragma once
#include <common/Base.hh>
#include <string>
#include <SDL3/SDL.h>

namespace Monoworks
{
	struct SWindowCreateInfos 
	{
		std::string	WindowTitle;
		SExtent2D	WindowExtent;
		u32			GraphicsAPI;
		bool		Resizable;
	};

	class CWindow
	{
	public:
		CWindow(const SWindowCreateInfos* pInfos) noexcept;
		void Shutdown() noexcept;


		[[nodiscard]] SDL_Window* Get() const noexcept { return m_SDLWindow; };
		[[nodiscard]] void* GetNative() const noexcept { return (void*)m_SDLWindow; };

		[[nodiscard]] u32 GetWidth() const noexcept { return m_WindowExtent.Width; }
		[[nodiscard]] u32 GetHeight() const noexcept { return m_WindowExtent.Height; }
		[[nodiscard]] SExtent2D GetExtent() const noexcept { return m_WindowExtent; }

	private:
		SDL_Window* m_SDLWindow;
		SExtent2D m_WindowExtent; 
	};
}
 