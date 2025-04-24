include(${PROJECT_SOURCE_DIR}/cmake/gen_protocol.cmake)

add_library(wayland-protocol)

set(PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/wayland.xml)

set(PROTOCOL_CODE ${CMAKE_CURRENT_BINARY_DIR}/wayland-protocol.c)
gen_protocol_source(
    PROTOCOL_XML ${PROTOCOL}
    PUBLIC
    OUTPUT_FILE ${PROTOCOL_CODE})

set(PROTOCOL_SERVER_CORE_HEADER ${PROJECT_BINARY_DIR}/include/wayland-server-protocol-core.h)
set(PROTOCOL_SERVER_HEADER ${PROJECT_BINARY_DIR}/include/wayland-server-protocol.h)
gen_protocol_header(
    SERVER CORE
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_SERVER_CORE_HEADER})
gen_protocol_header(
    SERVER
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_SERVER_HEADER})

set(PROTOCOL_CLIENT_CORE_HEADER ${PROJECT_BINARY_DIR}/include/wayland-client-protocol-core.h)
set(PROTOCOL_CLIENT_HEADER ${PROJECT_BINARY_DIR}/include/wayland-client-protocol.h)
gen_protocol_header(
    CLIENT CORE
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_CLIENT_CORE_HEADER})
gen_protocol_header(
    CLIENT
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_CLIENT_HEADER})

target_sources(wayland-protocol PRIVATE
    ${PROTOCOL_CODE}
    ${PROTOCOL_SERVER_CORE_HEADER}
    ${PROTOCOL_SERVER_HEADER}
    ${PROTOCOL_CLIENT_CORE_HEADER}
    ${PROTOCOL_CLIENT_HEADER})
target_link_libraries(wayland-protocol PRIVATE wayland-util)
target_include_directories(wayland-protocol PUBLIC
    ${PROJECT_BINARY_DIR}/include)
