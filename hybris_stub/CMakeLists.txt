project(hybris_stub)

add_library(hybris-common SHARED
        src/hybris-dlfcn.c)
target_include_directories(hybris-common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(hybris-common PROPERTIES SUFFIX ".so.1"
    POSITION_INDEPENDENT_CODE ON)

add_library(android-properties SHARED
        src/hybris-properties.c)
target_include_directories(android-properties PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(android-properties PROPERTIES SUFFIX ".so.1"
    POSITION_INDEPENDENT_CODE ON)
