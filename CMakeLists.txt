cmake_minimum_required(VERSION 3.12)
project(EGL_ext)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

option(SUPPORT_WAYLAND "" ON)

add_subdirectory(common)
add_subdirectory(gralloc)
add_subdirectory(egl_impl)
add_subdirectory(third_party)

# ## wayland platform
if(SUPPORT_WAYLAND)
    add_subdirectory(wayland)
    # add_subdirectory(platform/wayland/protocol)
endif()

