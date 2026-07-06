set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTOUIC ON) 
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_executable(MonoEditor "source/core/MonoEditor.hh" "source/core/MonoEditor.cc"  "source/core/EngineBridge.hh" "source/core/EngineBridge.cc" ${QRC_RESOURCEN} )

target_link_libraries(MonoEditor PRIVATE
    MonoEngine
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick 
)

target_include_directories(MonoEditor PRIVATE "source/" "specific/")

if (WIN32)
    target_compile_definitions(MonoEditor PRIVATE VT_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(MonoEditor PRIVATE VT_PLATFORM_LINUX)
elseif (APPLE)
    target_compile_definitions(MonoEditor PRIVATE VT_PLATFORM_OSX)
endif()

target_compile_definitions(MonoEditor PRIVATE MW_EDITOR)