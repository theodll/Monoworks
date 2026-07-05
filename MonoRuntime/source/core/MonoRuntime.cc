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
		CApplication* app = new CApplication;
		
		SApplicationCreateInfos appInfos{};
		appInfos.Name = "MonoEditor";
		appInfos.RenderableExtent = { 670, 480 };

		app->Init(&appInfos);


		app->Shutdown();

		return 0;
	}
}
