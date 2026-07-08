/**
 * @file LogManager.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Logging Manager
 * @version 0.1
 * @date 2026-07-08
 * @copyright Copyright (c) 2026
 * @ingroup Core
 */

#pragma once
#include <common/Memory.hh>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Monoworks 
{
    /**
     * @brief Subsystem for managing Logging
     */
    class CLogManager
    {
    public:
        /**
         * @brief Initializes the Logging Manager
         */
        static void Init();

        /**
         * @brief Flushes the Logs to disk
         */
        static void Flush();

        /**
         * @brief Set the Log File object
         * @param filepath Path to the Log file
         */
        static void SetLogFile(const std::string& filepath);

        /**
         * @brief Get the Core Logger object
         * @return std::shared_ptr<spdlog::logger> 
         */
        inline static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> s_FileSink;
    };
}