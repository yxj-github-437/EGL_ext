project(cutils_stub)

add_library(cutils SHARED
    src/native_handle.c)

target_include_directories(cutils PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
