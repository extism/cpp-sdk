cmake_minimum_required(VERSION 3.18)
include_guard(GLOBAL)
set (CMAKE_CXX_STANDARD 14)

# extism-cpp library
project(extism-cpp VERSION 1.0.0 DESCRIPTION "C++ bindings for libextism")
set(extism-cpp-srcs src/manifest.cpp src/current_plugin.cpp src/plugin.cpp src/function.cpp src/extism.cpp)

option(EXTISM_CPP_BUILD_IN_TREE "Set to ON to build with submodule deps" OFF)
option(EXTISM_CPP_WITH_CMAKE_PACKAGE "Generate and install cmake package files" ON)

if(EXTISM_CPP_BUILD_IN_TREE)
    message("EXTISM_CPP_BUILD_IN_TREE: using deps from parent directory")
    add_subdirectory(../extism/libextism extism)
    add_subdirectory(../jsoncpp jsoncpp)
else()
    include(FetchContent)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    find_package(extism)
    if(extism_FOUND)
        message("Found extism")
    else()
        message("Did not find extism, using FetchContent")
        FetchContent_Declare(
          extism
          GIT_REPOSITORY https://github.com/extism/extism.git
          GIT_TAG main
          SOURCE_SUBDIR libextism
        )
        FetchContent_MakeAvailable(extism)
    endif()
    find_package(jsoncpp)
    if(jsoncpp_FOUND)
        message("Found jsoncpp")
    else()
        message("Did not find jsoncpp, using FetchContent")
        FetchContent_Declare(
          jsoncpp
          GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
          GIT_TAG master
        )
        FetchContent_MakeAvailable(jsoncpp)
    endif()
endif()
find_package(GTest)

# SHARED
add_library(extism-cpp SHARED ${extism-cpp-srcs})
set_target_properties(extism-cpp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(extism-cpp PROPERTIES PUBLIC_HEADER src/extism.hpp)
target_include_directories(extism-cpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(extism-cpp PUBLIC extism-shared jsoncpp_lib)
set_target_properties(extism-cpp
  PROPERTIES NO_SONAME 1
)
configure_file(extism-cpp.pc.in extism-cpp.pc @ONLY)
list(APPEND CMAKE_TARGETS extism-cpp)

# STATIC
add_library(extism-cpp-static STATIC ${extism-cpp-srcs})
set_target_properties(extism-cpp-static PROPERTIES OUTPUT_NAME extism-cpp)
set_target_properties(extism-cpp-static PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(extism-cpp-static PROPERTIES PUBLIC_HEADER src/extism.hpp)
target_include_directories(extism-cpp-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(extism-cpp-static PUBLIC extism-static)
if(TARGET jsoncpp_static)
  target_link_libraries(extism-cpp-static PUBLIC jsoncpp_static)
else()
  message(WARNING "jsoncpp_static not found, linking jsoncpp_lib instead")
  target_link_libraries(extism-cpp-static PUBLIC jsoncpp_lib)
endif()
configure_file(extism-cpp-static.pc.in extism-cpp-static.pc @ONLY)
list(APPEND CMAKE_TARGETS extism-cpp-static)

if(EXTISM_CPP_WITH_CMAKE_PACKAGE)
    set(INSTALL_EXPORT EXPORT extism-cpp)
else()
    set(INSTALL_EXPORT)
endif()

include(GNUInstallDirs)
install(TARGETS ${CMAKE_TARGETS} ${INSTALL_EXPORT}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/extism-cpp.pc ${CMAKE_CURRENT_BINARY_DIR}/extism-cpp-static.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
if(EXTISM_CPP_WITH_CMAKE_PACKAGE)
    include(CMakePackageConfigHelpers)
    install(EXPORT extism-cpp
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/extism-cpp
        FILE        extism-cpp-targets.cmake)
    configure_package_config_file(extism-cppConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/extism-cppConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/extism-cpp)
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/extism-cppConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/extism-cppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/extism-cppConfigVersion.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/extism-cpp-namespaced-targets.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findextism.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/extism-cpp)
endif()

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
  GTest::gtest
  extism-cpp
)
include(GoogleTest)
add_test(NAME Test COMMAND extism-test)

