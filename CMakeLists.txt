cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0100 NEW)

project(Monoworks LANGUAGES CXX)
option(USE_SYSTEM_SDL3 "Use system-installed SDL3 via find_package" OFF)
#list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 23)

set(COMPILE_WARNING_AS_ERROR ON)

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
add_subdirectory(Common/extern/doctest)