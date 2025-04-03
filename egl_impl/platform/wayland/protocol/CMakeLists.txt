include(${PROJECT_SOURCE_DIR}/wayland/cmake/gen_protocol.cmake)

set(PROTOCOL_NAME wayland-android)
set(PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/${PROTOCOL_NAME}.xml)

set(PROTOCOL_CODE ${CMAKE_CURRENT_BINARY_DIR}/${PROTOCOL_NAME}-protocol.c)
gen_protocol_source(
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_CODE})

set(PROTOCOL_SERVER_CORE_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/${PROTOCOL_NAME}-server-protocol-core.h)
set(PROTOCOL_SERVER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/${PROTOCOL_NAME}-server-protocol.h)
gen_protocol_header(
    SERVER CORE
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_SERVER_CORE_HEADER})
gen_protocol_header(
    SERVER
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_SERVER_HEADER})

set(PROTOCOL_CLIENT_CORE_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/${PROTOCOL_NAME}-client-protocol-core.h)
set(PROTOCOL_CLIENT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/${PROTOCOL_NAME}-client-protocol.h)
gen_protocol_header(
    CLIENT CORE
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_CLIENT_CORE_HEADER})
gen_protocol_header(
    CLIENT
    PROTOCOL_XML ${PROTOCOL}
    OUTPUT_FILE ${PROTOCOL_CLIENT_HEADER})

add_library(${PROTOCOL_NAME}-protocol)
target_sources(${PROTOCOL_NAME}-protocol PRIVATE
        ${PROTOCOL_CODE}
        ${PROTOCOL_SERVER_CORE_HEADER}
        ${PROTOCOL_SERVER_HEADER}
        ${PROTOCOL_CLIENT_CORE_HEADER}
        ${PROTOCOL_CLIENT_HEADER})
target_include_directories(${PROTOCOL_NAME}-protocol PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(${PROTOCOL_NAME}-protocol PRIVATE wayland-util)
