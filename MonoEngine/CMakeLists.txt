cmake_policy(SET CMP0100 NEW)

add_library(MonoEngine STATIC "thirdparty/iniparser/src/dictionary.c" "thirdparty/iniparser/src/dictionary.h" "thirdparty/iniparser/src/iniparser.c" "thirdparty/iniparser/src/iniparser.h" "source/common/Math.hh" "source/core/Application.cc" "source/core/Application.hh" "Monoworks.hh" "source/common/Memory.hh" "source/core/MemoryManager.hh" "source/core/MemoryManager.cc" "source/core/LogManager.hh" "source/core/LogManager.cc" "source/common/Log.hh" "source/core/CVarManager.hh" "source/core/CVarManager.cc" "source/core/ConfigManager.hh" "source/core/ConfigManager.cc" "source/events/Event.hh" "source/common/SafeQueue.hh" "source/events/EventManager.hh" "source/events/EventManager.cc" "source/common/Events.hh" "source/jobs/Threadpool.hh" "source/jobs/Fibers.hh" "source/rhi/GraphicsAPI.hh" "source/rhi/specific/vulkan/VulkanContext.hh" "source/rhi/specific/vulkan/VulkanContext.cc" "source/rhi/agnostic/GraphicsContext.hh"  "source/rhi/specific/vulkan/VulkanDevice.hh" "source/rhi/specific/vulkan/VulkanDevice.cc"  "source/rhi/specific/vulkan/VulkanRenderer.hh" "source/rhi/specific/vulkan/VulkanRenderer.cc" "source/rhi/specific/vulkan/VulkanResourceUploader.hh" "source/renderer/StaticRenderer.hh" "source/renderer/StaticRenderer.cc" "source/rhi/specific/vulkan/VulkanResourceUploader.cc" "source/rhi/agnostic/VertexBuffer.hh" "source/rhi/agnostic/VertexBuffer.cc" "source/rhi/specific/vulkan/VulkanVertexBuffer.hh" "source/rhi/specific/vulkan/VulkanVertexBuffer.cc" "source/rhi/agnostic/IndexBuffer.cc" "source/rhi/specific/vulkan/VulkanIndexBuffer.hh" "source/rhi/specific/vulkan/VulkanIndexBuffer.cc" "source/rhi/agnostic/UniformBuffer.hh" "source/rhi/agnostic/UniformBuffer.cc" "source/rhi/specific/vulkan/VulkanUniformBuffer.hh" "source/rhi/specific/vulkan/VulkanUniformBuffer.cc" "source/rhi/agnostic/Texture.hh" "source/rhi/Utils.hh" "source/rhi/agnostic/Texture.cc" "source/rhi/specific/vulkan/VulkanTexture.hh" "source/rhi/specific/vulkan/VulkanTexture.cc" "source/rhi/agnostic/Presenter.hh" "source/renderer/SceneRenderer.hh" "source/rhi/specific/vulkan/VulkanPresenter.hh" "source/rhi/specific/vulkan/VulkanGraphicsPipeline.hh" "source/rhi/agnostic/GraphicsPipeline.hh" "source/rhi/agnostic/GraphicsPipeline.cc" "source/rhi/specific/vulkan/VulkanGraphicsPipeline.cc" "source/rhi/specific/vulkan/VulkanRenderManager.hh" "source/rhi/specific/vulkan/VulkanRenderManager.cc") 

set(GLM_BUILD_LIBRARY OFF CACHE INTERNAL "") 
add_subdirectory("thirdparty/glm")
# add_subdirectory("extern/imgui")
add_subdirectory("thirdparty/spdlog")
add_subdirectory("thirdparty/tracy")
add_subdirectory("thirdparty/volk")
add_subdirectory("thirdparty/stb")

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
    stb::stb
)

target_compile_definitions(MonoEngine PUBLIC VK_NO_PROTOTYPES)

target_precompile_headers(MonoEngine PUBLIC 
    $<$<COMPILE_LANGUAGE:CXX>:mwpch.hh>
)


if (WIN32)
    add_compile_definitions(MW_PLATFORM_WINDOWS)
    add_compile_definitions(
        VK_USE_PLATFORM_WIN32_KHR
        VMA_EXTERNAL_MEMORY_WIN32=1
    )
elseif (UNIX AND NOT APPLE)                       
    add_compile_definitions(MW_PLATFORM_LINUX)
elseif (APPLE)                                    
    target_compile_definitions(MW_PLATFORM_OSX)
endif()


get_directory_property(current_defs COMPILE_DEFINITIONS)
if ("MW_ENABLE_MANUAL_RENDERDOC" IN_LIST current_defs)
    if (WIN32)
     target_include_directories(MonoEngine PUBLIC "C:/Program Files/RenderDoc")
    endif()
endif()

target_compile_definitions(MonoEngine PRIVATE MW_ENGINE TRACY_ENABLE)