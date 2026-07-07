#include <core/LoggingManager.hh>

#define VT_TRACE(...)    if(::Velt::Log::GetCoreLogger()) ::Velt::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VT_INFO(...)     if(::Velt::Log::GetCoreLogger()) ::Velt::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VT_WARN(...)     if(::Velt::Log::GetCoreLogger()) ::Velt::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VT_ERROR(...)    if(::Velt::Log::GetCoreLogger()) ::Velt::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VT_FATAL(...)    if(::Velt::Log::GetCoreLogger()) ::Velt::Log::GetCoreLogger()->critical(__VA_ARGS_





