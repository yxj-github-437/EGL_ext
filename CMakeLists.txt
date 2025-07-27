cmake_minimum_required(VERSION 3.12)
project(EGL_ext)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

option(SUPPORT_WAYLAND "" ON)

# ## wayland platform
if(SUPPORT_WAYLAND)
    set(WAYLAND_BUILD_SCANNER OFF)
    add_subdirectory(wayland)
endif()

add_subdirectory(utils_common)
add_subdirectory(android_stub)
add_subdirectory(gralloc)
add_subdirectory(egl_impl)

