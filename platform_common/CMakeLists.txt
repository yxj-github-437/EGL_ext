if(SUPPORT_WAYLAND)
    add_subdirectory(wayland/protocol)
endif()


add_library(platform_common)
target_sources(platform_common PRIVATE
    base/platform_base.cc)
set_target_properties(platform_common PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(platform_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(platform_common PUBLIC
    sync
    utils_common)

if(SUPPORT_WAYLAND)
    target_sources(platform_common PRIVATE
        wayland/platform_wayland.cc)
    target_link_libraries(platform_common PUBLIC
        wayland-client)
endif()

