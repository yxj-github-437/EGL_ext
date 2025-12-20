add_library(egl_loader)
target_sources(egl_loader PRIVATE loader.cc)
target_include_directories(egl_loader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(egl_loader PUBLIC utils_common egl_platform)
