#include "EventDispatcher.hh"

#include <common/Base.hh>
#include <common/Events.h>

#include <events/EventManager.hh>

#include <SDL3/SDL.h>

namespace Monoworks 
{
	void CSDLEventDispatcher::Init()
	{
		MW_PROFILE_FUNC;
		MW_INFO("Initialize CSDLEventDispatcher");
	}


	void CSDLEventDispatcher::Shutdown()
	{
		MW_PROFILE_FUNC;
		MW_INFO("Shutdown CSDLEventDispatcher");
	}

	void CSDLEventDispatcher::ProcessEvents()
	{
		MW_PROFILE_FUNC;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_WINDOW_RESIZED:
			{
				Events::SWindowResize e{};
				e.NewExtent = { (u32)event.window.data1, (u32)event.window.data2 };
				CEventManager::EmitEvent(e, MW_EVENT_WINDOW_RESIZE);
				break;
			}
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			{
				Events::SWindowClose e{};
				CEventManager::EmitEvent(e, MW_EVENT_WINDOW_CLOSE);
				break;
			}
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
			{
				Events::SWindowFocus e{};
				CEventManager::EmitEvent(e, MW_EVENT_WINDOW_FOCUS);
				break;
			}
			case SDL_EVENT_WINDOW_FOCUS_LOST:
			{
				Events::SWindowLostFocus e{};
				CEventManager::EmitEvent(e, MW_EVENT_WINDOW_LOST_FOCUS);
				break;
			}
			case SDL_EVENT_WINDOW_MINIMIZED:
			{
				Events::SWindowMinimize e{};
				CEventManager::EmitEvent(e, MW_EVENT_WINDOW_MINIMIZE);
				break;
			}
			case SDL_EVENT_KEY_DOWN:
			{
				Events::SKeyPressed e{};
				e.Code = (Scancode)(u32)event.key.key;
				CEventManager::EmitEvent(e, MW_EVENT_KEY_PRESSED);
				break;
			}
			case SDL_EVENT_KEY_UP:
			{
				Events::SKeyReleased e{};
				e.Code = (Scancode)event.key.key;
				CEventManager::EmitEvent(e, MW_EVENT_KEY_RELEASED);
				break; 
			}
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			{
				Events::SMouseButtonPressed e{};
				switch (event.button.button)
				{
				case SDL_BUTTON_LEFT:
					e.Button = MW_MOUSE_L_BUTTON;
					break;

				case SDL_BUTTON_RIGHT:
					e.Button = MW_MOUSE_R_BUTTON;
					break;

				case SDL_BUTTON_MIDDLE:
					e.Button = MW_MOUSE_M_BUTTON;
					break;
				}

				e.MouseX = event.motion.x;
				e.MouseY = event.motion.y;
				CEventManager::EmitEvent(e, MW_EVENT_MOUSE_BUTTON_PRESSED);
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				Events::SMouseButtonReleased e{};
				switch (event.button.button)
				{
				case SDL_BUTTON_LEFT:
					e.Button = MW_MOUSE_L_BUTTON;
					break;

				case SDL_BUTTON_RIGHT:
					e.Button = MW_MOUSE_R_BUTTON;
					break;

				case SDL_BUTTON_MIDDLE:
					e.Button = MW_MOUSE_M_BUTTON;
					break;
				}

				e.MouseX = event.motion.x;
				e.MouseY = event.motion.y;
				CEventManager::EmitEvent(e, MW_EVENT_MOUSE_BUTTON_RELEASED);
				break;
			}
			case SDL_EVENT_MOUSE_MOTION:
			{
				Events::SMouseMoved e{};
				e.MouseX = event.motion.x;
				e.MouseY = event.motion.y;
				CEventManager::EmitEvent(e, MW_EVENT_MOUSE_MOVED);
				break;
			}
			case SDL_EVENT_MOUSE_WHEEL:
			{
				Events::SMouseScrolled e{};
				e.XOffset = event.motion.xrel;
				e.YOffset = event.motion.yrel;
				CEventManager::EmitEvent(e, MW_EVENT_MOUSE_SCROLLED);
				break;
			}

			}
		}

	}

}

