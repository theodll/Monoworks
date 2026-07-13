#pragma once
#include <Monoworks.hh>

namespace Monoworks 
{
	class CSDLEventDispatcher 
	{
	public:
		void Init();
		void Shutdown();


		void ProcessEvents();

	};
}
