cmake_policy(SET CMP0100 NEW)

set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTOUIC ON) 
set(CMAKE_AUTORCC ON)

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
endif()

find_package(
    Qt6
    REQUIRED COMPONENTS 
    Core 
    Gui
    Qml
    Quick
    Widgets
    OpenGLWidgets)

add_subdirectory(thirdparty/KDDockWidgets)

add_executable(MonoEditor "source/core/MonoEditor.hh" "source/core/MonoEditor.cc" "source/rhi/specific/vulkan/VulkanQtPresenter.hh" ${QRC_RESOURCEN}  "source/widgets/ViewportWidget.cpp" "source/widgets/ViewportWidget.h" "source/core/EngineManager.h" "source/core/EngineManager.cpp" "source/rhi/specific/vulkan/VulkanQtPresenter.cpp")

target_link_libraries(MonoEditor PRIVATE
    MonoEngine
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick 
    Qt6::Widgets
    Qt6::OpenGLWidgets
    KDAB::kddockwidgets
)

target_include_directories(MonoEditor PRIVATE "source/" "specific/")

if (WIN32)
    target_compile_definitions(MonoEditor PRIVATE MW_PLATFORM_WINDOWS VK_USE_PLATFORM_WIN32_KHR WIN32_LEAN_AND_MEAN )
elseif (UNIX AND NOT APPLE)                       
    target_compile_definitions(MonoEditor PRIVATE MW_PLATFORM_LINUX)
elseif (APPLE)                                    
    target_compile_definitions(MonoEditor PRIVATE MW_PLATFORM_OSX)
endif()

target_compile_definitions(MonoEditor PRIVATE MW_EDITOR)