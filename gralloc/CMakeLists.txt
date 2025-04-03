project(gralloc)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_library(gralloc
    src/gralloc_adapter.cc
    src/GrallocUsageConversion.cc)

target_include_directories(gralloc PUBLIC
    ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(gralloc PUBLIC
    common
    hardware    # stub
    nativebase  # stub
    cutils      # stub

    dl)


add_executable(gralloc_test test.cc)
target_link_libraries(gralloc_test gralloc)

add_executable(gralloc_socket_test socket_test.cc)
target_link_libraries(gralloc_socket_test gralloc)
