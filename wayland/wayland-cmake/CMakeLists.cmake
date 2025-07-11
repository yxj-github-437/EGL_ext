cmake_minimum_required(VERSION ${CMAKE_VERSION})

set(CMAKE_C_STANDARD 99)

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/meson.build
    VERSION_CONTENT REGEX "version: '([0-9]+)\\.([0-9]+)\\.([0-9]+)'")
string(REGEX MATCHALL "[0-9]+" VERSION_LIST ${VERSION_CONTENT})

list(GET VERSION_LIST 0 VERSION_0)
list(GET VERSION_LIST 1 VERSION_1)
list(GET VERSION_LIST 2 VERSION_2)
set(WAYLAND_VERSION_MAJOR ${VERSION_0})
set(WAYLAND_VERSION_MINOR ${VERSION_1})
set(WAYLAND_VERSION_MICRO ${VERSION_2})
set(WAYLAND_VERSION ${VERSION_0}.${VERSION_1}.${VERSION_2})
message(STATUS "wayland version: ${WAYLAND_VERSION}")

if(WAYLAND_BUILD_SCANNER AND Python3_EXECUTABLE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/wayland.dtd.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src/embed.py
            ${CMAKE_CURRENT_SOURCE_DIR}/protocol/wayland.dtd wayland_dtd >
            ${CMAKE_CURRENT_BINARY_DIR}/include/wayland.dtd.h
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/embed.py
            ${CMAKE_CURRENT_SOURCE_DIR}/protocol/wayland.dtd)
endif()

if(WAYLAND_BUILD_LIBRARIES)
    if(FFI_SHARED_LIB)
        set(FFI_LIB ffi)
    elseif(FFI_STATIC_LIB)
        set(FFI_LIB ffi_static)
        add_link_options(-Wl,--exclude-libs,libffi.a)
    endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-version.h.in
    ${PROJECT_BINARY_DIR}/include/wayland-version.h @ONLY)

include(CheckFunctionExists)
include(CheckIncludeFile)

check_function_exists(accept4 HAVE_ACCEPT4)
set(HAVE_BROKEN_MSG_CMSG_CLOEXEC 0)
check_function_exists(memfd_create HAVE_MEMFD_CREATE)
check_function_exists(mkostemp HAVE_MKOSTEMP)
check_function_exists(mremap HAVE_MREMAP)
check_function_exists(posix_fallocate HAVE_POSIX_FALLOCATE)
check_function_exists(prctl HAVE_PRCTL)
check_function_exists(strndup HAVE_STRNDUP)
check_include_file(sys/prctl.h HAVE_SYS_PRCTL_H)
check_include_file(sys/procctl.h HAVE_SYS_PROCCTL_H)
check_include_file(sys/ucred.h HAVE_SYS_UCRED_H)
set(HAVE_XUCRED_CR_PID 0)
set(PACKAGE wayland)
set(PACKAGE_VERSION ${WAYLAND_VERSION})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${PROJECT_BINARY_DIR}/config.h @ONLY)

add_compile_options(
    -Wno-unused-parameter
    -Wstrict-prototypes
    -Wmissing-prototypes
    -fvisibility=hidden
    -D_POSIX_C_SOURCE=200809L)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-util.h
    ${PROJECT_BINARY_DIR}/include/wayland-util.h @ONLY)
add_library(wayland-util
    src/wayland-util.c)
target_include_directories(wayland-util PUBLIC
    ${PROJECT_BINARY_DIR}/include)

if(ANDROID)
    target_link_libraries(wayland-util PRIVATE log)
endif()

if(WAYLAND_BUILD_SCANNER)
    add_executable(wayland-scanner
        src/scanner.c)

    target_link_libraries(wayland-scanner
        wayland-util
        expat)
    target_link_options(wayland-scanner PRIVATE -Wl,--as-needed)

    if(Python3_EXECUTABLE)
        target_compile_definitions(wayland-scanner PRIVATE -DHAVE_LIBXML=1)
        target_sources(wayland-scanner PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/include/wayland.dtd.h)
        target_include_directories(wayland-scanner PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/include)
        target_link_libraries(wayland-scanner LibXml2)
    endif()

    set_property(TARGET wayland-scanner
        PROPERTY RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin)
endif()

if(CMAKE_CROSSCOMPILING OR NOT WAYLAND_BUILD_SCANNER)
    include(host/host-build.cmake)
else()
    add_executable(wayland::scanner ALIAS wayland-scanner)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/protocol/wayland.xml
    ${PROJECT_BINARY_DIR}/protocol/wayland.xml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/protocol/wayland.dtd
    ${PROJECT_BINARY_DIR}/protocol/wayland.dtd COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wayland-scanner.mk
    ${PROJECT_BINARY_DIR}/protocol/wayland-scanner.mk COPYONLY)

if(WAYLAND_BUILD_LIBRARIES)
    add_link_options(-Wl,--as-needed)

    add_library(wayland-private
        src/connection.c
        src/wayland-os.c
        ${PROJECT_SOURCE_DIR}/wayland-cmake/open_memstream.c)
    target_link_libraries(wayland-private PRIVATE
        ${FFI_LIB}
        $<$<PLATFORM_ID:Android>:log>)
    target_include_directories(wayland-private PUBLIC
        ${PROJECT_BINARY_DIR}/include)

    # generate protocol source/headers from protocol XMLs
    add_subdirectory(protocol)

    if(WAYLAND_BUILD_SERVER)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-server.h
            ${PROJECT_BINARY_DIR}/include/wayland-server.h @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-server-core.h
            ${PROJECT_BINARY_DIR}/include/wayland-server-core.h @ONLY)

        add_library(wayland-server SHARED
            src/wayland-server.c
            src/wayland-shm.c
            src/event-loop.c)
        target_link_libraries(wayland-server PRIVATE
            wayland-protocol
            wayland-util
            wayland-private)
        target_include_directories(wayland-server
            PUBLIC
            ${PROJECT_BINARY_DIR}/include
            PRIVATE
            ${PROJECT_BINARY_DIR})
        set_property(TARGET wayland-server
            PROPERTY LIBRARY_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/lib)
    endif()

    if(WAYLAND_BUILD_CLIENT)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-client.h
            ${PROJECT_BINARY_DIR}/include/wayland-client.h @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-client-core.h
            ${PROJECT_BINARY_DIR}/include/wayland-client-core.h @ONLY)

        add_library(wayland-client SHARED
            src/wayland-client.c)
        target_link_libraries(wayland-client PRIVATE
            wayland-protocol
            wayland-util
            wayland-private)
        target_include_directories(wayland-client PUBLIC
            ${PROJECT_BINARY_DIR}/include)
        set_property(TARGET wayland-client
            PROPERTY LIBRARY_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/lib)

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/egl/wayland-egl.h
            ${PROJECT_BINARY_DIR}/include/wayland-egl.h @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/egl/wayland-egl-backend.h
            ${PROJECT_BINARY_DIR}/include/wayland-egl-backend.h @ONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/egl/wayland-egl-core.h
            ${PROJECT_BINARY_DIR}/include/wayland-egl-core.h @ONLY)

        add_library(wayland-egl SHARED
            egl/wayland-egl.c)
        target_link_libraries(wayland-egl
            wayland-client)
        set_property(TARGET wayland-egl
            PROPERTY LIBRARY_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/lib)

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cursor/wayland-cursor.h
            ${PROJECT_BINARY_DIR}/include/wayland-cursor.h @ONLY)

        add_library(wayland-cursor SHARED
            cursor/wayland-cursor.c
            cursor/os-compatibility.c
            cursor/xcursor.c)
        target_link_libraries(wayland-cursor
            wayland-client)
        target_include_directories(wayland-cursor PRIVATE
            ${PROJECT_BINARY_DIR})
        set_property(TARGET wayland-cursor
            PROPERTY LIBRARY_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/lib)
    endif()
endif()
