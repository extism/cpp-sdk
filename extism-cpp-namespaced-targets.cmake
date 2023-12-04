if (TARGET extism-cpp-static)
    add_library(ExtismCpp:ExtismCpp INTERFACE IMPORTED)
    set_target_properties(ExtismCpp:ExtismCpp PROPERTIES INTERFACE_LINK_LIBRARIES "extism-cpp-static")
elseif (TARGET extism-cpp-shared)
    add_library(ExtismCpp:ExtismCpp INTERFACE IMPORTED)
    set_target_properties(ExtismCpp:ExtismCpp PROPERTIES INTERFACE_LINK_LIBRARIES "extism-cpp-shared")
endif ()