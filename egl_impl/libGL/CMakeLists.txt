project (gl4es)

add_library(GL SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/gl.cc)
set_target_properties(GL PROPERTIES SUFFIX ".so.1"
    POSITION_INDEPENDENT_CODE ON)
target_link_libraries(GL PRIVATE
    EGL

    utils_common)
target_include_directories(GL
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(GL PRIVATE
    -fno-exceptions
    -fvisibility=hidden)
target_compile_definitions(GL PRIVATE
    GL_GLEXT_PROTOTYPES)
