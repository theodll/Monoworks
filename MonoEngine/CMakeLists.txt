cmake_policy(SET CMP0100 NEW)

add_library(MonoEngine STATIC "source/common/Math.hh" "source/core/Application.cc" "source/core/Application.hh" "Monoworks.hh")

add_subdirectory("thirdparty/glm")
# add_subdirectory("extern/imgui")
add_subdirectory("thirdparty/spdlog")
add_subdirectory("thirdparty/tracy")
add_subdirectory("thirdparty/volk")

find_package(Vulkan REQUIRED)

target_include_directories(MonoEngine PUBLIC "source/" "specific/" ".")

target_link_libraries(MonoEngine PRIVATE
    Vulkan::Vulkan
    spdlog::spdlog
    Tracy::TracyClient
    volk::volk
)

target_link_libraries(MonoEngine PUBLIC
    doctest::doctest
    glm::glm
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