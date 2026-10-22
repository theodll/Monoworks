#include <Monoworks.hh>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW( GetCommandLineW(), &argc );
	auto res =  Monoworks::MonoCrashReporterMain( argc, argv );
	LocalFree( argv );
	return res;
};


namespace Monoworks 
{


	int MWAPI_CALL MonoCrashReporterMain( int argc, wchar_t** pArgv ) NOEXCEPT
	{

	}

}
