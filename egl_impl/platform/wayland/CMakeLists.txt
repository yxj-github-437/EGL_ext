add_subdirectory(protocol)

add_library(egl_platform_wayland)
target_sources(egl_platform_wayland PRIVATE
    platform_wayland.cc)
target_link_libraries(egl_platform_wayland PUBLIC
    egl_platform_base
    wayland-android-protocol
    wayland-server
    wayland-client
    wayland-egl)
