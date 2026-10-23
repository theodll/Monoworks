#pragma once
#include <iostream>
#include <vector>

#include "MonoCrashReporter.hh"

namespace Monoworks 
{
	using pid_t = uint32_t;

	pid_t GetPID( ::std::string_view processName ) noexcept;
	::std::vector<uint32_t> EnumeratePIDs() noexcept;

	#define MW_CRASH_REPORTER_INFO(fmtstr, ...) std::println( std::cout, "[CRASH-REPORT PROCESS]: {}", fmtstr __VA_OPT__(,) __VA_ARGS__);
	#define MW_CRASH_REPORTER_ERROR(fmtstr, ...) std::println( std::cerr, "[CRASH-REPORT PROCESS]: {}", fmtstr __VA_OPT__(,) __VA_ARGS__);


}

