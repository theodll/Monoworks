/**
 * @file Log.h
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Macro Definitions for Logging
 * @version 0.1
 * @date 2026-07-08
 * @ingroup Common
 * @copyright Copyright (c) 2026
 * 
 */

#include <core/LogManager.hh>

#define VT_TRACE(...)    if(::Velt::CLogManager::GetCoreLogger()) ::Velt::CLogManager::GetCoreLogger()->trace(__VA_ARGS__)
#define VT_INFO(...)     if(::Velt::CLogManager::GetCoreLogger()) ::Velt::CLogManager::GetCoreLogger()->info(__VA_ARGS__)
#define VT_WARN(...)     if(::Velt::CLogManager::GetCoreLogger()) ::Velt::CLogManager::GetCoreLogger()->warn(__VA_ARGS__)
#define VT_ERROR(...)    if(::Velt::CLogManager::GetCoreLogger()) ::Velt::CLogManager::GetCoreLogger()->error(__VA_ARGS__)
#define VT_FATAL(...)    if(::Velt::CLogManager::GetCoreLogger()) ::Velt::CLogManager::GetCoreLogger()->critical(__VA_ARGS_)





