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
#include <string_view>

constexpr std::string_view StripFunctionName( std::string_view name )
{
    // Compilergenerierte Suffixe entfernen: "::<lambda_7>::operator ()" etc.
    if ( const auto pos = name.find( "::<" ); pos != std::string_view::npos )
        name = name.substr( 0, pos );

    // Namespace-Präfix entfernen
    constexpr std::string_view prefix = "Monoworks::";
    if ( name.starts_with( prefix ) )
        name.remove_prefix( prefix.size() );

    return name;
}

#define MW_LOG(fn, fmt_str, ...) \
    do { \
        if (auto logger = ::Monoworks::CLogManager::GetCoreLogger()) { \
            logger->fn("[{}] " fmt_str, StripFunctionName(__FUNCTION__) __VA_OPT__(,) __VA_ARGS__); \
        } \
        else { \
            fmt::print("[{}] " fmt_str "\n", StripFunctionName(__FUNCTION__) __VA_OPT__(,) __VA_ARGS__); \
        } \
    } while (0)

#define MW_TRACE(fmt_str, ...) MW_LOG(trace,    fmt_str __VA_OPT__(,) __VA_ARGS__)
#define MW_INFO(fmt_str, ...)  MW_LOG(info,     fmt_str __VA_OPT__(,) __VA_ARGS__)
#define MW_WARN(fmt_str, ...)  MW_LOG(warn,     fmt_str __VA_OPT__(,) __VA_ARGS__)
#define MW_ERROR(fmt_str, ...) MW_LOG(error,    fmt_str __VA_OPT__(,) __VA_ARGS__)
#define MW_FATAL(fmt_str, ...) do { MW_LOG(critical, fmt_str __VA_OPT__(,) __VA_ARGS__); std::exit(1); } while (0)

#define MW_API_WARN(fmt, ...)  MW_WARN ("Invalid API usage: " fmt __VA_OPT__(,) __VA_ARGS__)
#define MW_API_ERROR(fmt, ...) MW_ERROR("Invalid API usage: " fmt __VA_OPT__(,) __VA_ARGS__)
#define MW_API_FATAL(fmt, ...) MW_FATAL("Invalid API usage: " fmt __VA_OPT__(,) __VA_ARGS__)