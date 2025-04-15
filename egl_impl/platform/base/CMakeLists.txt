add_library(egl_platform_base)
target_sources(egl_platform_base PRIVATE
    platform_base.cc)
target_link_libraries(egl_platform_base PUBLIC
    sync
    gralloc)
target_include_directories(egl_platform_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set_target_properties(egl_platform_base PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    INTERFACE_POSITION_INDEPENDENT_CODE ON)
