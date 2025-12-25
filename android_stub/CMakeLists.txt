project(android_stub)

add_library(sync SHARED
    src/sync_stub.c)
target_include_directories(sync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(hardware SHARED
    src/hardware_stub.c)
target_include_directories(hardware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(cutils INTERFACE)
target_include_directories(cutils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
