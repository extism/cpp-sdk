cmake_policy(SET CMP0053 OLD)
find_package(PkgConfig)
pkg_check_modules(PC_jsoncpp QUIET jsoncpp)
find_path(jsoncpp_INCLUDE_DIR
  NAMES json/json.h json.h
  PATHS ${PC_jsoncpp_INCLUDE_DIRS}
)
find_library(jsoncpp_LIBRARY
  NAMES jsoncpp
  PATHS ${PC_jsoncpp_LIBRARY_DIRS}
)
find_library(jsoncpp_STATIC_LIBRARY
  NAMES libjsoncpp.a
  PATHS ${PC_jsoncpp_LIBRARY_DIRS}
)
set(jsoncpp_VERSION ${PC_jsoncpp_VERSION})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jsoncpp
  FOUND_VAR jsoncpp_FOUND
  REQUIRED_VARS
    jsoncpp_LIBRARY
    jsoncpp_INCLUDE_DIR
  VERSION_VAR jsoncpp_VERSION
)

if(jsoncpp_FOUND)
  set(jsoncpp_LIBRARIES ${jsoncpp_LIBRARY})
  set(jsoncpp_INCLUDE_DIRS ${jsoncpp_INCLUDE_DIR})
  set(jsoncpp_DEFINITIONS ${PC_jsoncpp_CFLAGS_OTHER})
endif()
if(jsoncpp_FOUND AND NOT TARGET jsoncpp_lib)
  add_library(jsoncpp_lib UNKNOWN IMPORTED)
  set_target_properties(jsoncpp_lib PROPERTIES
    IMPORTED_LOCATION "${jsoncpp_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_jsoncpp_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${jsoncpp_INCLUDE_DIR}"
  )
endif()

if(jsoncpp_FOUND AND NOT jsoncpp_STATIC_LIBRARY-NOTFOUND AND NOT TARGET jsoncpp_static)
  add_library(jsoncpp_static UNKNOWN IMPORTED)
  set_target_properties(jsoncpp_static PROPERTIES
    IMPORTED_LOCATION "${jsoncpp_STATIC_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_jsoncpp_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${jsoncpp_INCLUDE_DIR}"
  )
endif()
mark_as_advanced(
  jsoncpp_INCLUDE_DIR
  jsoncpp_LIBRARY
  jsoncpp_STATIC_LIBRARY
)
