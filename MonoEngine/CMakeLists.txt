cmake_policy(SET CMP0100 NEW)

add_library(MonoEngine STATIC "source/common/Math.hh" "source/core/Application.cc" "source/core/Application.hh" "Monoworks.hh" "source/common/Memory.hh" "source/core/MemoryManager.hh" "source/core/MemoryManager.cc" "source/core/LogManager.hh" "source/core/LogManager.cc" "source/common/Log.hh" "source/core/CVarManager.hh" "source/core/CVarManager.cc" "source/core/ConfigManager.hh" "source/core/ConfigManager.cc" "source/events/Event.hh" "source/common/SafeQueue.hh" "source/events/EventManager.hh" "source/events/EventManager.cc" "source/common/Events.hh" "source/jobs/Threadpool.hh" "source/jobs/Fibers.hh" "source/rhi/GraphicsAPI.hh" "source/rhi/specific/vulkan/VulkanContext.hh" "source/rhi/specific/vulkan/VulkanContext.cc" "source/rhi/agnostic/GraphicsContext.hh"  "source/rhi/specific/vulkan/VulkanDevice.hh" "source/rhi/specific/vulkan/VulkanDevice.cc"  "source/rhi/specific/vulkan/VulkanRenderer.hh" "source/rhi/specific/vulkan/VulkanRenderer.cc" "source/rhi/specific/vulkan/VulkanResourceUploader.hh" "source/renderer/StaticRenderer.hh" "source/renderer/StaticRenderer.cc" "source/rhi/specific/vulkan/VulkanResourceUploader.cc" "source/rhi/agnostic/VertexBuffer.hh" "source/rhi/agnostic/VertexBuffer.cc" "source/rhi/specific/vulkan/VulkanVertexBuffer.hh" "source/rhi/specific/vulkan/VulkanVertexBuffer.cc" "source/rhi/agnostic/IndexBuffer.cc" "source/rhi/specific/vulkan/VulkanIndexBuffer.hh" "source/rhi/specific/vulkan/VulkanIndexBuffer.cc" "source/rhi/agnostic/UniformBuffer.hh" "source/rhi/agnostic/UniformBuffer.cc" "source/rhi/specific/vulkan/VulkanUniformBuffer.hh" "source/rhi/specific/vulkan/VulkanUniformBuffer.cc" "source/rhi/agnostic/Texture.hh" "source/rhi/Utils.hh" "source/rhi/agnostic/Texture.cc" "source/rhi/specific/vulkan/VulkanTexture.hh" "source/rhi/specific/vulkan/VulkanTexture.cc" "source/rhi/agnostic/Presenter.hh" "source/renderer/SceneRenderer.hh" "source/rhi/specific/vulkan/VulkanPresenter.hh" "source/rhi/specific/vulkan/VulkanGraphicsPipeline.hh" "source/rhi/agnostic/GraphicsPipeline.hh" "source/rhi/agnostic/GraphicsPipeline.cc" "source/rhi/specific/vulkan/VulkanGraphicsPipeline.cc" "source/rhi/specific/vulkan/VulkanRenderManager.hh" "source/rhi/specific/vulkan/VulkanRenderManager.cc" "source/renderer/FrameManager.hh" "source/renderer/FrameManager.cc" "source/renderer/FrameGraph.hh" "source/renderer/Shader.hh" "source/renderer/Shader.cc" "source/rhi/agnostic/PipelineManager.hh" "source/rhi/agnostic/ComputePipeline.Hh" "source/rhi/specific/vulkan/VulkanComputePipeline.hh" "source/rhi/specific/vulkan/VulkanComputePipeline.cpp" "source/rhi/agnostic/PipelineManager.cc") 

set(GLM_BUILD_LIBRARY OFF CACHE INTERNAL "") 
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable Slang tests" FORCE)
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable Slang examples" FORCE)
set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable legacy GFX layer" FORCE)
set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "Disable Slang RHI" FORCE)
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable Slang language server" FORCE)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.92.0 REQUIRED) 


add_subdirectory("thirdparty/glm")
# add_subdirectory("extern/imgui")
add_subdirectory("thirdparty/spdlog")
add_subdirectory("thirdparty/tracy")
add_subdirectory("thirdparty/volk")
add_subdirectory("thirdparty/stb")
add_subdirectory("thirdparty/slang")


target_include_directories(MonoEngine SYSTEM PUBLIC "extern/VulkanMemoryAllocator/include")
add_subdirectory("extern/VulkanMemoryAllocator")

find_package(Vulkan REQUIRED)

target_include_directories(MonoEngine PUBLIC "source/" "specific/" "." "thirdparty/")

add_library(iniparser STATIC
    thirdparty/iniparser/src/dictionary.c
    thirdparty/iniparser/src/iniparser.c
 )

target_include_directories(iniparser PUBLIC "thirdparty/iniparser/src/")


target_link_libraries(MonoEngine PUBLIC
    spdlog::spdlog 
    Tracy::TracyClient
    volk::volk
    glm::glm
    stb::stb
    iniparser
    slang
    Boost::headers
)

target_compile_definitions(MonoEngine PUBLIC VK_NO_PROTOTYPES)

target_precompile_headers(MonoEngine PUBLIC 
    $<$<COMPILE_LANGUAGE:CXX>:mwpch.hh>
)

enable_testing()
add_executable(EngineTests tests/core/TestMain.cpp tests/core/ConfigManagerTests.cc tests/core/EventSystemTests.cpp tests/core/CommonTests.cpp )
target_compile_options(EngineTests PRIVATE "-UMW_PROFILING")

target_link_libraries( EngineTests PRIVATE doctest::doctest MonoEngine )

target_include_directories( EngineTests PRIVATE "${CMAKE_SOURCE_DIR}/Common/extern/doctest")

include(CTest)
add_test(NAME EngineTest COMMAND EngineTests)

if (WIN32)
    add_compile_definitions(MW_PLATFORM_WINDOWS)
    add_compile_definitions(
        VK_USE_PLATFORM_WIN32_KHR
        VMA_EXTERNAL_MEMORY_WIN32=1
    )
elseif (UNIX AND NOT APPLE)                       
    add_compile_definitions(MW_PLATFORM_LINUX)
elseif (APPLE)                                    
    add_compile_definitions(MW_PLATFORM_OSX)
endif()

if (MSVC)
    target_compile_options(MonoEngine PRIVATE
        /W4
        /WX
    )

    target_compile_options(MonoEngine PUBLIC /EHsc)
else()
target_compile_options(MonoEngine PRIVATE
    -Wall
    -Wextra
    -Werror
)
endif()

get_directory_property(current_defs COMPILE_DEFINITIONS)
if ("MW_ENABLE_MANUAL_RENDERDOC" IN_LIST current_defs)
    if (WIN32)
     target_include_directories(MonoEngine PUBLIC "C:/Program Files/RenderDoc")
    endif()
endif()

target_compile_definitions(MonoEngine PRIVATE MW_ENGINE TRACY_ENABLE)