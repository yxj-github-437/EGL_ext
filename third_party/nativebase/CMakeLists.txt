project(nativebase)

add_library(nativebase INTERFACE)

target_include_directories(nativebase INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(nativebase INTERFACE
    cutils)
