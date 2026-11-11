#pragma once
#include <iostream>
#include <format>
#include <print>
#include <vector>

#include "MonoCrashReporter.hh"

namespace Monoworks 
{
	typedef  uint32_t pid_t;
	typedef  unsigned char byte_t;

#define MW_CRASH_REPORTER_INFO(fmtstr, ...) std::println(std::cout, fmtstr __VA_OPT__(,) __VA_ARGS__)
#define MW_CRASH_REPORTER_ERROR(fmtstr, ...) std::println(std::cerr, fmtstr __VA_OPT__(,) __VA_ARGS__)
}

