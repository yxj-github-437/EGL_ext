add_executable(wayland::scanner-cross IMPORTED GLOBAL)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

if(NOT WAYLAND_SCANNER_EXECUTABLE)
    if(CMAKE_HOST_WIN32)
        set(EXECUTABLE_SUFFIX .exe)
    endif()

    set(HOST_WAYLAND_SCANNER_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/wayland-scanner.cross)
    set(HOST_WAYLAND_SCANNER_BINARY ${CMAKE_CURRENT_BINARY_DIR}/wayland-scanner.cross-build)

    file(MAKE_DIRECTORY ${HOST_WAYLAND_SCANNER_SOURCE})

    configure_file(${PROJECT_BINARY_DIR}/include/wayland-version.h
        ${HOST_WAYLAND_SCANNER_SOURCE}/wayland-version.h COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-private.h
        ${HOST_WAYLAND_SCANNER_SOURCE}/wayland-private.h COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-util.h
        ${HOST_WAYLAND_SCANNER_SOURCE}/wayland-util.h COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-util.c
        ${HOST_WAYLAND_SCANNER_SOURCE}/wayland-util.c COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.c
        ${HOST_WAYLAND_SCANNER_SOURCE}/scanner.c COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/host/host-CMakeLists.cmake
        ${HOST_WAYLAND_SCANNER_SOURCE}/CMakeLists.txt COPYONLY)

    file(CREATE_LINK ${PROJECT_SOURCE_DIR}/libexpat ${HOST_WAYLAND_SCANNER_SOURCE}/libexpat SYMBOLIC)

    set(WAYLAND_SCANNER_EXECUTABLE ${HOST_WAYLAND_SCANNER_BINARY}/wayland-scanner${EXECUTABLE_SUFFIX})

    include(ExternalProject)
    ExternalProject_Add(wayland-scanner.cross
        SOURCE_DIR ${HOST_WAYLAND_SCANNER_SOURCE}
        BINARY_DIR ${HOST_WAYLAND_SCANNER_BINARY}
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
        BUILD_BYPRODUCTS ${WAYLAND_SCANNER_EXECUTABLE}
        UPDATE_COMMAND ""
        INSTALL_COMMAND "")

    add_dependencies(wayland::scanner-cross wayland-scanner.cross)
endif()

set_target_properties(wayland::scanner-cross PROPERTIES
    IMPORTED_LOCATION ${WAYLAND_SCANNER_EXECUTABLE})
