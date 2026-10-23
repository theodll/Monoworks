#include <Windows.h>
#include <core/MonoCrashReporter.hh>

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW( GetCommandLineW(), &argc );
	auto res = Monoworks::MonoCrashReporterMain( argc, argv );
	LocalFree( argv );
	return res;
};

namespace Monoworks
{
	int MWAPI_CALL MonoCrashReporterMain( int argc, wchar_t** pArgv ) NOEXCEPT
	{
		CMonoCrashReporter rp;
		rp.Init();
	}


	struct CMonoCrashReporter::Impl
	{
		HANDLE HeartbeatPipe;
		HANDLE EngineProcess; 

	};

	void MWAPI_CALL CMonoCrashReporter::Init() NOEXCEPT
	{
		
	}


	

}
