

add_library(MonoEngine STATIC "source/common/Math.hh" "source/core/Application.cc" "source/core/Application.hh" "Monoworks.hh")

add_subdirectory("extern/glm")
# add_subdirectory("extern/imgui")
add_subdirectory("extern/spdlog")
add_subdirectory("extern/tracy")
add_subdirectory("extern/volk")

find_package(Vulkan REQUIRED)

target_include_directories(MonoEngine PUBLIC "source/" "specific/")

target_link_libraries(MonoEngine PRIVATE
    Vulkan::Vulkan
    spdlog::spdlog
    Tracy::TracyClient
    volk::volk
    doctest::doctest
)

target_link_libraries(MonoEngine PUBLIC
    doctest::doctest
)

target_precompile_headers(MonoEngine PUBLIC mwpch.hh)

if (WIN32)
    target_compile_definitions(MonoEngine PRIVATE VT_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(MonoEngine PRIVATE VT_PLATFORM_LINUX)
elseif (APPLE)
    target_compile_definitions(MonoEngine PRIVATE VT_PLATFORM_OSX)
endif()

target_compile_definitions(MonoEngine PRIVATE MW_ENGINE)