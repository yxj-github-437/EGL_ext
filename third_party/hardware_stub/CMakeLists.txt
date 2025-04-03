project(hardware_stub)

add_library(hardware SHARED
    src/hardware.c)

target_link_libraries(hardware
    nativebase)

target_include_directories(hardware PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
