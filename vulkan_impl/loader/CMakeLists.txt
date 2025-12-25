add_library(vulkan_loader)
target_sources(vulkan_loader PRIVATE loader.cc)
target_include_directories(vulkan_loader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(vulkan_loader PUBLIC utils_common)
