add_library(vulkan_platform)
target_sources(vulkan_platform PRIVATE)
target_include_directories(vulkan_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(vulkan_platform PUBLIC
    gralloc
    utils_common
    platform_common
    vulkan_loader)

if (SUPPORT_WAYLAND)
    target_sources(vulkan_platform PRIVATE
        wayland/platform_wayland.cc)
    target_include_directories(vulkan_platform PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland)
    target_link_libraries(vulkan_platform PUBLIC
        wayland-android-protocol
        wayland-egl
        wayland-client)
endif()
