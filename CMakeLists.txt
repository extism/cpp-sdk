cmake_minimum_required(VERSION 3.9)
set (CMAKE_CXX_STANDARD 14)
include_directories(/usr/local/include)
include_directories(/opt/homebrew/include)
link_directories(/usr/local/lib)
link_directories(/opt/homebrew/lib)

# extism-cpp library
project(extism-cpp VERSION 1.0.0 DESCRIPTION "C++ bindings for libextism")
add_library(extism-cpp SHARED src/manifest.cpp src/current_plugin.cpp src/plugin.cpp src/function.cpp src/extism.cpp)
set_target_properties(extism-cpp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(extism-cpp PROPERTIES PUBLIC_HEADER src/extism.hpp)
target_include_directories(extism-cpp PUBLIC src)
target_link_libraries(extism-cpp PUBLIC extism jsoncpp)
set_target_properties(extism-cpp
  PROPERTIES NO_SONAME 1
)
install(TARGETS extism-cpp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Example
add_executable(
  example
  example.cpp
)
target_link_libraries(
  example
  extism-cpp
)

# Tests
enable_testing()
add_executable(
  extism-test
  test/test.cpp
)
target_link_directories(extism-test PUBLIC ${CURRENT_BINARY_DIR})
target_link_libraries(
  extism-test
  gtest
  extism-cpp
)
include(GoogleTest)
add_test(NAME Test COMMAND extism-test)

