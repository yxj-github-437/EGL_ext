cmake_minimum_required(VERSION 3.12)
project(EGL_ext)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

option(SUPPORT_WAYLAND "" ON)

# ## wayland platform
if(SUPPORT_WAYLAND)
    set(WAYLAND_BUILD_SCANNER OFF)
    add_subdirectory(wayland)
endif()

add_subdirectory(common)
add_subdirectory(android_stub)
add_subdirectory(gralloc)
add_subdirectory(egl_impl)

