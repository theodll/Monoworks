cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0100 NEW)

project(Monoworks LANGUAGES C CXX ASM)
if(NOT CMAKE_CONFIGURATION_TYPES AND CMAKE_BUILD_TYPE)
    file(MAKE_DIRECTORY
        "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
        "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
        "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib")
endif()

option(USE_SYSTEM_SDL3 "Use system-installed SDL3 via find_package" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin-int)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin-int)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin-int)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(COMPILE_WARNING_AS_ERROR ON)

add_compile_definitions(MW_PROFILING MW_VULKAN TRACY_ENABLE TRACY_ON_DEMAND)

option(MW_ENABLE_AVX2 "Compile with AVX2" ON)
if (MSVC)
    add_compile_options(/Zc:preprocessor)
endif()
if (MW_ENABLE_AVX2 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    if (MSVC)
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-mavx2)
    endif()
endif()

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
    add_compile_definitions(MW_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(MW_PLATFORM_OSX)
elseif(UNIX)
    add_compile_definitions(MW_PLATFORM_LINUX)
endif()

include(FetchContent)
set(BOOST_INCLUDE_LIBRARIES "" CACHE STRING "" FORCE)

FetchContent_Declare(
    boost
    URL https://sourceforge.net/projects/boost/files/boost/1.92.0/boost_1_92_0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    FIND_PACKAGE_ARGS 1.92.0 CONFIG
)
FetchContent_MakeAvailable(boost)

if(NOT TARGET Boost::headers)
    add_library(Boost::headers INTERFACE IMPORTED GLOBAL)
    target_include_directories(Boost::headers SYSTEM INTERFACE "${boost_SOURCE_DIR}")
endif()

# add_compile_definitions(MW_ENABLE_MANUAL_RENDERDOC)

add_compile_definitions(
    FMT_UNICODE=0
    $<$<CONFIG:Debug>:MW_DEBUG>
    $<$<CONFIG:Release>:MW_RELEASE>
    $<$<CONFIG:Dist>:MW_DIST>
)

add_subdirectory(reporter)
add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(runtime)

if(EXISTS "${CMAKE_SOURCE_DIR}/common")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    file(GLOB common_entries RELATIVE "${CMAKE_SOURCE_DIR}/common" "${CMAKE_SOURCE_DIR}/common/*")

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