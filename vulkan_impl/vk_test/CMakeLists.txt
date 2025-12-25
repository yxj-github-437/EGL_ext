add_subdirectory(xdg-shell)

add_executable(vulkan_test vk_test.cc)
target_link_libraries(vulkan_test PRIVATE
    vulkan
    xdg-shell-protocol
    wayland-client)
