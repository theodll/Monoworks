#include <Monoworks.hh>
#include <core/Application.hh>
#include "MonoRuntime.hh"


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
		m_Application = new CApplication;

		SApplicationCreateInfos appInfos{};
		appInfos.Name = "Mono Runtime";
		appInfos.RenderableExtent = { 640, 480 };
		appInfos.ArgumentCount = argc;
		appInfos.Arguments = argv;

		m_Application->Init(&appInfos);

		SWindowCreateInfos windowInfos{};
		windowInfos.GraphicsAPI = MW_GAPI_NONE;
		windowInfos.WindowTitle = "Mono Runtime";
		windowInfos.Resizable = false;
		windowInfos.WindowExtent = { 640, 480 };

		m_Window = Ref<CWindow>::Create();
		m_Window->Init(&windowInfos);

		Ref<int> test = Ref<int>::Create();
	}

	void CMonoRuntime::Run()
	{
		while(m_Running) 
		{
			SDL_Event event;
			while(SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_EVENT_QUIT:
					m_Running = false;
				}
			}
			m_Application->Frame();
		}
	}

	void CMonoRuntime::Shutdown()
	{

		m_Window->Shutdown();
		m_Application->Shutdown();
	}

}
