project(android_stub)

add_library(sync INTERFACE)
target_include_directories(sync INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(hardware INTERFACE)
target_include_directories(hardware INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(cutils INTERFACE)
target_include_directories(cutils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
