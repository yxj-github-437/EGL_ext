add_library(egl_loader)
target_sources(egl_loader PRIVATE loader/loader.cc)
target_include_directories(egl_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(egl_loader PUBLIC common egl_platform)

add_library(egl_platform)
target_sources(egl_platform PRIVATE
    platform/egl_object.cc
    platform/egl_platform_entries.cc
    platform/egl_tls.cc)
target_include_directories(egl_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(egl_platform PUBLIC common)
target_compile_definitions(egl_platform PRIVATE
    EGL_EGLEXT_PROTOTYPES
    GL_GLEXT_PROTOTYPES)

add_subdirectory(platform/base)
if (SUPPORT_WAYLAND)
    add_subdirectory(platform/wayland)
    target_link_libraries(egl_platform PUBLIC egl_platform_wayland)
endif()

add_library(EGL SHARED
    libEGL/eglApi.cc)
set_target_properties(EGL PROPERTIES SUFFIX ".so.1")
target_link_libraries(EGL PRIVATE
    egl_loader
    egl_platform

    gralloc
    sync
    dl)
target_compile_options(EGL PRIVATE
    -fno-exceptions
    -fvisibility=hidden)
target_compile_definitions(EGL PRIVATE
    EGL_EGLEXT_PROTOTYPES
    GL_GLEXT_PROTOTYPES)

add_library(GLESv1_CM SHARED
    libGLESv1_CM/gl.cc)
set_target_properties(GLESv1_CM PROPERTIES SUFFIX ".so.1")
target_link_libraries(GLESv1_CM PRIVATE
    EGL
    common)
target_include_directories(GLESv1_CM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(GLESv1_CM PRIVATE
    -fno-exceptions
    -fvisibility=hidden)
target_compile_definitions(GLESv1_CM PRIVATE
    GL_GLEXT_PROTOTYPES)


add_library(GLESv2 SHARED
    libGLESv2/gl2.cc)
set_target_properties(GLESv2 PROPERTIES SUFFIX ".so.2")
target_link_libraries(GLESv2 PRIVATE
    EGL

    common)
target_include_directories(GLESv2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(GLESv2 PRIVATE
    -fno-exceptions
    -fvisibility=hidden)
target_compile_definitions(GLESv2 PRIVATE
    GL_GLEXT_PROTOTYPES)

add_executable(egl_test test.cc)
target_link_libraries(egl_test PUBLIC EGL GLESv2)


add_executable(egl_fbo_pbo_test fbo_pbo_test.cc)
target_link_libraries(egl_fbo_pbo_test PUBLIC EGL GLESv2)

add_executable(egl_fbo_rbo_test fbo_rbo_test.cc)
target_link_libraries(egl_fbo_rbo_test PUBLIC EGL GLESv2)
