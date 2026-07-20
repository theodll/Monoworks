cmake_policy(SET CMP0100 NEW)

add_library(MonoEngine STATIC "thirdparty/iniparser/src/dictionary.c" "thirdparty/iniparser/src/dictionary.h" "thirdparty/iniparser/src/iniparser.c" "thirdparty/iniparser/src/iniparser.h" "source/common/Math.hh" "source/core/Application.cc" "source/core/Application.hh" "Monoworks.hh" "source/common/Memory.hh" "source/core/MemoryManager.hh" "source/core/MemoryManager.cc" "source/core/LogManager.hh" "source/core/LogManager.cc" "source/common/Log.h" "source/core/CVarManager.hh" "source/core/CVarManager.cc" "source/core/ConfigManager.hh" "source/core/ConfigManager.cc" "source/events/Event.hh" "source/common/SafeQueue.hh" "source/events/EventManager.hh" "source/events/EventManager.cc" "source/common/Events.h" "source/jobs/Threadpool.hh" "source/jobs/Fibers.hh" "source/rhi/GraphicsAPI.hh" "source/rhi/specific/vulkan/VulkanContext.hh" "source/rhi/agnostic/GraphicsContext.hh"  "source/rhi/specific/vulkan/VulkanDevice.h" "source/rhi/specific/vulkan/VulkanDevice.cpp" "source/rhi/specific/vulkan/VulkanContext.cpp" "source/rhi/specific/vulkan/VulkanRenderer.hh" "source/rhi/specific/vulkan/VulkanResourceUploader.hh" "source/rhi/specific/vulkan/VulkanResourceUploader.cc")

set(GLM_BUILD_LIBRARY OFF CACHE INTERNAL "") 
add_subdirectory("thirdparty/glm")
# add_subdirectory("extern/imgui")
add_subdirectory("thirdparty/spdlog")
add_subdirectory("thirdparty/tracy")
add_subdirectory("thirdparty/volk")

target_include_directories(MonoEngine PUBLIC "extern/VulkanMemoryAllocator/include")
add_subdirectory("extern/VulkanMemoryAllocator")

find_package(Vulkan REQUIRED)

target_include_directories(MonoEngine PUBLIC "source/" "specific/" "." "thirdparty/iniparser/src/" "thirdparty/")

target_link_libraries(MonoEngine PUBLIC
    spdlog::spdlog 
    Tracy::TracyClient
    volk::volk
    doctest::doctest
    glm::glm
)

target_compile_definitions(MonoEngine PUBLIC VK_NO_PROTOTYPES)

target_precompile_headers(MonoEngine PUBLIC 
    $<$<COMPILE_LANGUAGE:CXX>:mwpch.hh>
)

if (WIN32)
    target_compile_definitions(MonoEngine PRIVATE MW_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)                       
    target_compile_definitions(MonoEngine PRIVATE MW_PLATFORM_LINUX)
elseif (APPLE)                                    
    target_compile_definitions(MonoEngine PRIVATE MW_PLATFORM_OSX)
endif()

target_compile_definitions(MonoEngine PRIVATE MW_ENGINE TRACY_ENABLE)