add_library(egl_platform_base)
target_sources(egl_platform_base PRIVATE
    platform_base.cc)
target_link_libraries(egl_platform_base PUBLIC
    nativebase
    sync
    gralloc)
target_include_directories(egl_platform_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
