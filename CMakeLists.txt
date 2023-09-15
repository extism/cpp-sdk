cmake_minimum_required(VERSION 3.9)

project(extism-cpp VERSION 1.0.0 DESCRIPTION "C++ bindings for libextism")
add_library(extism-cpp SHARED extism.cpp)
set_target_properties(extism-cpp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(extism-cpp PROPERTIES PUBLIC_HEADER extism.hpp)
include_directories(${PROJECT_SOURCE_DIR}/jsoncpp)
target_include_directories(extism-cpp PRIVATE src)
target_link_libraries(extism-cpp PRIVATE jsoncpp)
install(TARGETS extism-cpp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

