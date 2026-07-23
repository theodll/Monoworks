#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	class CSceneRenderer 
	{
	public:
		void Init();
		void Shutdown();

		void BeginScene();
		void EndScene();
	};
}
