

if (WIN32)
    add_executable( MonoCrashReporter "source/core/win32/MonoCrashReporterWin32.cc" )
endif()

target_include_directories( MonoCrashReporter PRIVATE "source/" "../engine/source/" "../engine/" )

target_link_libraries( MonoCrashReporter PRIVATE Boost::headers )


if (MSVC)
    target_compile_options(MonoCrashReporter PRIVATE
        /W4
        /WX
    )
else()
target_compile_options(MonoCrashReporter PRIVATE
    -Wall
    -Wextra
    -Werror
)
endif()

if (WIN32)
    target_compile_definitions(MonoCrashReporter PRIVATE MW_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)                        
    target_compile_definitions(MonoCrashReporter PRIVATE MW_PLATFORM_LINUX)
elseif (APPLE)                                     
    target_compile_definitions(MonoCrashReporter PRIVATE MW_PLATFORM_OSX)
endif()

target_compile_definitions(MonoCrashReporter PRIVATE MW_CRASH_REPORTER TRACY_ENABLE)