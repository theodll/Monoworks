add_executable(MonoRuntime "source/core/Entrypoint.hh" "source/core/Entrypoint.cc")

if(APPLE)
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
else()
    add_subdirectory("extern/sdl")
endif()

target_link_libraries(MonoRuntime PRIVATE MonoEngine SDL3::SDL3)
target_include_directories(MonoRuntime PRIVATE "source/" "specific/")

if (WIN32)
    target_compile_definitions(MonoRuntime PRIVATE VT_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(MonoRuntime PRIVATE VT_PLATFORM_LINUX)
elseif (APPLE)
    target_compile_definitions(MonoRuntime PRIVATE VT_PLATFORM_OSX)
endif()

target_compile_definitions(MonoRuntime PRIVATE MW_RUNTIME)