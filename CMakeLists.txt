cmake_minimum_required(VERSION 3.5)

project(Monoworks LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    add_compile_definitions(MW_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(MW_PLATFORM_OSX)
elseif(UNIX)
    add_compile_definitions(MW_PLATFORM_LINUX)
endif()

add_compile_definitions(
    FMT_UNICODE=0
    $<$<CONFIG:Debug>:MW_DEBUG>
    $<$<CONFIG:Release>:MW_RELEASE>
    $<$<CONFIG:Dist>:MW_DIST>
)

add_subdirectory(MonoEngine)
add_subdirectory(MonoEditor)
add_subdirectory(MonoRuntime)
