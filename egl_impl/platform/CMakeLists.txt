add_library(egl_platform)
target_sources(egl_platform PRIVATE
    egl_object.cc
    egl_platform_entries.cc
    egl_tls.cc)
target_include_directories(egl_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(egl_platform PUBLIC
    gralloc
    utils_common
    platform_common
    egl_loader)

target_compile_definitions(egl_platform PRIVATE
    EGL_EGLEXT_PROTOTYPES
    GL_GLEXT_PROTOTYPES)

if (SUPPORT_WAYLAND)
    target_sources(egl_platform PRIVATE
        wayland/platform_wayland.cc)
    target_include_directories(egl_platform PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland)
    target_link_libraries(egl_platform PUBLIC
        wayland-android-protocol
        wayland-egl
        wayland-server
        wayland-client)
endif()
