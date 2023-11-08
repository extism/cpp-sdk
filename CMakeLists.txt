cmake_minimum_required(VERSION 3.9)
set (CMAKE_CXX_STANDARD 14)
include_directories(/usr/local/include)
include_directories(/opt/homebrew/include)
link_directories(/usr/local/lib)
link_directories(/opt/homebrew/lib)

# extism-cpp library
project(extism-cpp VERSION 1.0.0 DESCRIPTION "C++ bindings for libextism")
set(extism-cpp-srcs src/manifest.cpp src/current_plugin.cpp src/plugin.cpp src/function.cpp src/extism.cpp)

option(EXTISM_CPP_BUILD_IN_TREE "Set to ON to build with submodule deps" OFF)
if(EXTISM_CPP_BUILD_IN_TREE)
    add_subdirectory(submodules/extism/libextism)
    add_subdirectory(submodules/jsoncpp)
endif()

# SHARED
add_library(extism-cpp SHARED ${extism-cpp-srcs})
set_target_properties(extism-cpp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(extism-cpp PROPERTIES PUBLIC_HEADER src/extism.hpp)
target_include_directories(extism-cpp PUBLIC src)
if(EXTISM_CPP_BUILD_IN_TREE)
    target_link_libraries(extism-cpp PUBLIC extism-shared jsoncpp)
else()
    target_link_libraries(extism-cpp PUBLIC extism jsoncpp)
endif()
set_target_properties(extism-cpp
  PROPERTIES NO_SONAME 1
)
configure_file(extism-cpp.pc.in extism-cpp.pc @ONLY)

# STATIC
add_library(extism-cpp-static STATIC ${extism-cpp-srcs})
set_target_properties(extism-cpp-static PROPERTIES OUTPUT_NAME extism-cpp)
set_target_properties(extism-cpp-static PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(extism-cpp-static PROPERTIES PUBLIC_HEADER src/extism.hpp)
target_include_directories(extism-cpp-static PUBLIC src)
if(EXTISM_CPP_BUILD_IN_TREE)
    target_link_libraries(extism-cpp-static PUBLIC extism-static jsoncpp_static)
else()
    target_link_libraries(extism-cpp-static PUBLIC libextism.a jsoncpp)
endif()
configure_file(extism-cpp-static.pc.in extism-cpp-static.pc @ONLY)

include(GNUInstallDirs)
install(TARGETS extism-cpp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS extism-cpp-static
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/extism-cpp.pc ${CMAKE_CURRENT_BINARY_DIR}/extism-cpp-static.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# Example
add_executable(
  example
  example.cpp
)
target_link_libraries(
  example
  extism-cpp
)

# Static-ish example
add_executable(
  example-static
  example.cpp
)
target_link_libraries(
  example-static
  extism-cpp-static
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

