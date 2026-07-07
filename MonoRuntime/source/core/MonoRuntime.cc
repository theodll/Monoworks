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
		appInfos.Name = "MonoEditor";
		appInfos.RenderableExtent = { 670, 480 };
		appInfos.ArgumentCount = argc;
		appInfos.Arguments = argv;

		m_Application->Init(&appInfos);
	}

	void CMonoRuntime::Run()
	{
		while(m_Running) 
		{
			m_Application->Frame();
		}
	}

	void CMonoRuntime::Shutdown()
	{
		m_Application->Shutdown();
	}

}
