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
#include <source_location>

// #define MW_TRACE(...)    if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->trace(__VA_ARGS__)
// #define MW_INFO(...)     if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->info(__VA_ARGS__)
// #define MW_WARN(...)     if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->warn(__VA_ARGS__)
// #define MW_ERROR(...)    if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->error(__VA_ARGS__)
// #define MW_FATAL(...)    if(::Monoworks::CLogManager::GetCoreLogger()) ::Monoworks::CLogManager::GetCoreLogger()->critical(__VA_ARGS_)

constexpr std::string_view StripNamespace(std::string_view name) {
    constexpr std::string_view prefix = "Monoworks::";
    if (name.substr(0, prefix.size()) == prefix)
        return name.substr(prefix.size());
    return name;
}

#define MW_LOG(fn, fmt_str, ...) \
    do { \
        if (auto logger = ::Monoworks::CLogManager::GetCoreLogger()) { \
            logger->fn("[{}] " fmt_str, StripNamespace(__FUNCTION__), ##__VA_ARGS__); \
        } \
        else { \
            fmt::print("[{}] " fmt_str "\n", StripNamespace(__FUNCTION__), ##__VA_ARGS__); \
        } \
    } while (0)

#define MW_TRACE(fmt, ...) MW_LOG(trace,    fmt, ##__VA_ARGS__)
#define MW_INFO(fmt, ...)  MW_LOG(info,     fmt, ##__VA_ARGS__)
#define MW_WARN(fmt, ...)  MW_LOG(warn,     fmt, ##__VA_ARGS__)
#define MW_ERROR(fmt, ...) MW_LOG(error,    fmt, ##__VA_ARGS__)
#define MW_FATAL(fmt, ...) MW_LOG(critical, fmt, ##__VA_ARGS__)



