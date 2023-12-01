find_package(PkgConfig)
pkg_check_modules(PC_extism QUIET extism)
find_path(extism_INCLUDE_DIR
  NAMES extism.h
  PATHS ${PC_extism_INCLUDE_DIRS}
)
find_library(extism_LIBRARY
  NAMES extism
  PATHS ${PC_extism_LIBRARY_DIRS}
)
find_library(extism_STATIC_LIBRARY
  NAMES libextism.a
  PATHS ${PC_extism_LIBRARY_DIRS}
)
set(extism_VERSION ${PC_extism_VERSION})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(extism
  FOUND_VAR extism_FOUND
  REQUIRED_VARS
    extism_LIBRARY
    extism_STATIC_LIBRARY
    extism_INCLUDE_DIR
  VERSION_VAR extism_VERSION
)

if(extism_FOUND)
  set(extism_LIBRARIES ${extism_LIBRARY})
  set(extism_INCLUDE_DIRS ${extism_INCLUDE_DIR})
  set(extism_DEFINITIONS ${PC_extism_CFLAGS_OTHER})
endif()
if(extism_FOUND AND NOT TARGET extism-shared)
  add_library(extism-shared UNKNOWN IMPORTED)
  set_target_properties(extism-shared PROPERTIES
    IMPORTED_LOCATION "${extism_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_extism_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${extism_INCLUDE_DIR}"
  )
endif()
if(extism_FOUND AND NOT TARGET extism-static)
  add_library(extism-static UNKNOWN IMPORTED)
  set_target_properties(extism-static PROPERTIES
    IMPORTED_LOCATION "${extism_STATIC_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_extism_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${extism_INCLUDE_DIR}"
  )
endif()
mark_as_advanced(
  extism_INCLUDE_DIR
  extism_LIBRARY
  extism_STATIC_LIBRARY
)
