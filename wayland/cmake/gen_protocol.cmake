if (NOT TARGET wayland::scanner-cross)
    set(WAYLAND_SCANNER_TARGET wayland::scanner)
else()
    set(WAYLAND_SCANNER_TARGET wayland::scanner-cross)
endif()

function(gen_protocol_header)
    set(options SERVER CLIENT CORE)
    set(oneValueArgs PROTOCOL_XML OUTPUT_FILE)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ARG_CORE)
        set(CORE_FLAG -c)
    endif()

    if(ARG_SERVER AND NOT ARG_CLIENT)
        add_custom_command(
            OUTPUT ${ARG_OUTPUT_FILE}
            COMMAND ${WAYLAND_SCANNER_TARGET} -s server-header ${CORE_FLAG} ${ARG_PROTOCOL_XML} ${ARG_OUTPUT_FILE}
            DEPENDS ${WAYLAND_SCANNER_TARGET} ${ARG_PROTOCOL_XML}
            COMMENT "generate ${ARG_OUTPUT_FILE}")
    elseif(NOT ARG_SERVER AND ARG_CLIENT)
        add_custom_command(
            OUTPUT ${ARG_OUTPUT_FILE}
            COMMAND ${WAYLAND_SCANNER_TARGET} -s client-header ${CORE_FLAG} ${ARG_PROTOCOL_XML} ${ARG_OUTPUT_FILE}
            DEPENDS ${WAYLAND_SCANNER_TARGET} ${ARG_PROTOCOL_XML}
            COMMENT "generate ${ARG_OUTPUT_FILE}")
    endif()
endfunction()

function(gen_protocol_source)
    set(options PUBLIC)
    set(oneValueArgs PROTOCOL_XML OUTPUT_FILE)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(CODE_TYPE private-code)

    if(ARG_PUBLIC)
        set(CODE_TYPE public-code)
    endif()

    add_custom_command(
        OUTPUT ${ARG_OUTPUT_FILE}
        COMMAND ${WAYLAND_SCANNER_TARGET} -s ${CODE_TYPE} ${ARG_PROTOCOL_XML} ${ARG_OUTPUT_FILE}
        DEPENDS ${WAYLAND_SCANNER_TARGET} ${ARG_PROTOCOL_XML}
        COMMENT "generate ${ARG_OUTPUT_FILE}")
endfunction()
