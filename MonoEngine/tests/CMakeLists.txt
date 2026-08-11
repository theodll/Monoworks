cmake_minimum_required(VERSION 3.20)

enable_testing()
add_executable(EngineTests core/ConfigManagerTests.cc)

target_link_directories( EngineTests PRIVATE doctest::doctest EngineTests )

include(CTest)
add_test(NAME EngineTest COMMAND EngineTests)