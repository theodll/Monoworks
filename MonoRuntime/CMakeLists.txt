cmake_policy(SET CMP0100 NEW)

add_executable(MonoRuntime "source/core/MonoRuntime.hh" "source/core/MonoRuntime.cc" "source/core/Window.hh" "source/core/Window.cc" "specific/sdl/EventDispatcher.hh" "specific/sdl/EventDispatcher.cc" "source/rhi/specific/vulkan/VulkanSDLPresenter.hh" "source/rhi/specific/vulkan/VulkanSDLPresenter.cc")

if(MW_USE_SYSTEM_SDL3)
    find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
else()
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/sdl)
endif()

target_include_directories(MonoRuntime PRIVATE 
    "source/" 
    "specific/"
)

target_link_libraries(MonoRuntime PRIVATE 
    MonoEngine 
    SDL3::SDL3 
    Tracy::TracyClient
)

target_link_libraries(MonoRuntime PRIVATE MonoEngine SDL3::SDL3 Tracy::TracyClient)


target_include_directories(MonoRuntime PRIVATE "source/" "specific/" "extern/sdl/include")

if (WIN32)
    target_compile_definitions(MonoRuntime PRIVATE MW_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)                        
    target_compile_definitions(MonoRuntime PRIVATE MW_PLATFORM_LINUX)
elseif (APPLE)                                     
    target_compile_definitions(MonoRuntime PRIVATE MW_PLATFORM_OSX)
endif()

target_compile_definitions(MonoRuntime PRIVATE MW_RUNTIME TRACY_ENABLE)