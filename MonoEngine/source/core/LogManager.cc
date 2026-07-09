#include <mwpch.hh>

#include "core/LogManager.hh"



namespace Monoworks 
{
    std::shared_ptr<spdlog::logger> CLogManager::s_CoreLogger;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> CLogManager::s_FileSink;

    void CLogManager::Init()
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        //consoleSink->set_pattern("[%T] %n: %^[%l] %v%$");
        consoleSink->set_pattern("[%T : %n] %^[%l]%$ %v");

        s_FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Monoworks.log", true);
        s_FileSink->set_pattern("[%T] [%l] %n: %v");

        std::vector<spdlog::sink_ptr> coreSinks = { consoleSink, s_FileSink };
        s_CoreLogger = std::make_shared<spdlog::logger>("Monoworks", coreSinks.begin(), coreSinks.end());
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(s_CoreLogger);

        MW_INFO("Initialize CLogManager");
    }

    void CLogManager::Flush()
    {
        if (s_CoreLogger)
            s_CoreLogger->flush();
    }

    void CLogManager::SetLogFile(const std::string& filepath)
    {
        s_FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, true);
        s_FileSink->set_pattern("[%T] [%l] %n: %v");
    }

    void CLogManager::Shutdown()
    {
        MW_INFO("Shutdown CLogManager");
        Flush();
    }

}