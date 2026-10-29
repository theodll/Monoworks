
add_executable( MonoCrashReporter "source/core/MonoCrashReporter.hh"  "source/core/Utils.hh" "source/core/Utils.cc" "source/core/MonoCrashReporter.cpp")

if (WIN32)
#set_target_properties(MonoCrashReporter PROPERTIES WIN32_EXECUTABLE TRUE )
target_sources( MonoCrashReporter PRIVATE "source/core/win32/MonoCrashReporterWin32.cc" "source/core/win32/UtilsWin32.cc"  "source/core/win32/main.cpp")
endif()

target_include_directories( MonoCrashReporter PRIVATE "source/" )

target_link_libraries( MonoCrashReporter PRIVATE Boost::headers )


if (MSVC)
    target_compile_options(MonoCrashReporter PRIVATE
        /W4
        /WX
        /EHsc
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