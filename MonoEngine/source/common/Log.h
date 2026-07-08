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
#pragma once
#include <core/LogManager.hh>

#define MW_TRACE(...)    if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->trace(__VA_ARGS__)
#define MW_INFO(...)     if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->info(__VA_ARGS__)
#define MW_WARN(...)     if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->warn(__VA_ARGS__)
#define MW_ERROR(...)    if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->error(__VA_ARGS__)
#define MW_FATAL(...)    if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->critical(__VA_ARGS_)





