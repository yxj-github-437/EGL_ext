add_subdirectory(loader)
add_subdirectory(platform)

add_library(vulkan SHARED
    libvulkan/vulkan.cc)
set_target_properties(vulkan PROPERTIES SUFFIX ".so.1"
    POSITION_INDEPENDENT_CODE ON)
target_link_libraries(vulkan PRIVATE
    vulkan_loader
    vulkan_platform

    gralloc
    sync
    dl)
target_compile_options(vulkan PRIVATE
    -fno-exceptions
    -fvisibility=hidden)
target_include_directories(vulkan PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(vk_test)
