#include <core/Utils.hh>

#include <Windows.h>
#include <psapi.h>

#include <ranges>
#include <vector>

namespace Monoworks 
{
	std::vector<pid_t> EnumeratePIDs() noexcept
	{
		std::vector<DWORD> pids(1024);

		DWORD bytesReturned = 0;

		do 
		{
			pids.resize( pids.size() * 2 );
			if ( K32EnumProcesses( pids.data(), ( DWORD )pids.size() * sizeof DWORD, &bytesReturned ) == 0 )
			{
				return {};
			}
		} 
		while ( bytesReturned == pids.size() / sizeof(DWORD) );

		return pids
			| std::views::take(bytesReturned / sizeof DWORD )
			| std::views::transform( []( auto pid ) -> pid_t { return pid; } )
			| std::ranges::to<std::vector>();
	};
}
