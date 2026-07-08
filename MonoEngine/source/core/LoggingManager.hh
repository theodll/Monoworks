#include <common/Memory.hh>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Monoworks 
{
    class CLog
    {
    public:
        static void Init();
        static void Flush();
        static void SetLogFile(const std::string& filepath);

        inline static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> s_FileSink;
    };
}