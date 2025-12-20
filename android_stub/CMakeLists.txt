project(android_stub)

add_library(sync SHARED
    src/sync_stub.c)
target_include_directories(sync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(hardware SHARED
    src/hardware_stub.c)
target_include_directories(hardware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(cutils SHARED
    src/native_handle.c
    src/cutils_stub.c)
target_include_directories(cutils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WITH_HYBRIS)
    add_library(hybris-common SHARED
        src/hybris-dlfcn.c)
    target_include_directories(hybris-common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
