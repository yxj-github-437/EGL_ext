cmake_minimum_required(VERSION 3.12)
project(EGL_ext)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

option(WITH_HYBRIS     "" OFF)
option(SUPPORT_WAYLAND "" ON)
set(CMAKE_SKIP_RPATH TRUE)

# ## wayland platform
if(SUPPORT_WAYLAND)
    set(WAYLAND_BUILD_SCANNER OFF)
    add_subdirectory(wayland)
endif()

add_subdirectory(utils_common)
add_subdirectory(android_stub)
add_subdirectory(gralloc)
add_subdirectory(platform_common)
add_subdirectory(egl_impl)

