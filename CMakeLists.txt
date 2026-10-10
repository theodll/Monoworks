cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0100 NEW)

project(Monoworks LANGUAGES C CXX ASM)
option(USE_SYSTEM_SDL3 "Use system-installed SDL3 via find_package" OFF)
#list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin-int)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin-int)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin-int)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(COMPILE_WARNING_AS_ERROR ON)

add_compile_definitions(MW_PROFILING MW_VULKAN)

if (MSVC)
    add_compile_options(/Zc:preprocessor)
endif()

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
    set(BOOST_ROOT "C:/local/boost_1_92_0")
    add_compile_definitions(MW_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(MW_PLATFORM_OSX)
elseif(UNIX)
    add_compile_definitions(MW_PLATFORM_LINUX)
endif()

add_compile_definitions(TRACY_ENABLE)

# add_compile_definitions(MW_ENABLE_MANUAL_RENDERDOC)



add_compile_definitions(
    FMT_UNICODE=0
    $<$<CONFIG:Debug>:MW_DEBUG>
    $<$<CONFIG:Release>:MW_RELEASE>
    $<$<CONFIG:Dist>:MW_DIST>
)

add_subdirectory(MonoEngine)
add_subdirectory(MonoEditor)
add_subdirectory(MonoRuntime)
add_subdirectory(extern/doctest)

# Use a Python script to copy directories inside the project-level `common/`
# into each executable target's runtime directory. The Python script lives in
# `scripts/copy_common.py` and performs the actual copy; CMake only invokes it
# after the target is built.
if(EXISTS "${CMAKE_SOURCE_DIR}/common")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    # gather all entries under common (files and directories)
    file(GLOB common_entries RELATIVE "${CMAKE_SOURCE_DIR}/common" "${CMAKE_SOURCE_DIR}/common/*")

    # get all known targets
    get_property(_all_targets GLOBAL PROPERTY TARGETS)

    foreach(_tgt IN LISTS _all_targets)
        get_target_property(_type ${_tgt} TYPE)
        if(_type STREQUAL "EXECUTABLE")
            if(common_entries)
                add_custom_command(TARGET ${_tgt} POST_BUILD
                    COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/copy_common.py"
                        --source-dir "${CMAKE_SOURCE_DIR}/common"
                        --dest-dir "$<TARGET_FILE_DIR:${_tgt}>"
                    COMMENT "Running copy_common.py for target ${_tgt}"
                )
            endif()
        endif()
    endforeach()
endif()
