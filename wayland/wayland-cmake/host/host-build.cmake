find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

if (NOT WAYLAND_SCANNER_EXECUTABLE)
    if (CMAKE_HOST_WIN32)
        set(EXECUTABLE_SUFFIX .exe)
    endif()

    include(ExternalProject)
    ExternalProject_Add(wayland-scanner.cross
        PREFIX wayland-scanner.cross
        INSTALL_DIR ${PROJECT_BINARY_DIR}/host
        DOWNLOAD_COMMAND
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/include/wayland-version.h <SOURCE_DIR>/wayland-version.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-private.h <SOURCE_DIR>/wayland-private.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-util.h <SOURCE_DIR>/wayland-util.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/wayland-util.c <SOURCE_DIR>/wayland-util.c
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.c <SOURCE_DIR>/scanner.c
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/host/host-CMakeLists.cmake <SOURCE_DIR>/CMakeLists.txt
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/libexpat/ <SOURCE_DIR>/libexpat/
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
        BUILD_BYPRODUCTS <INSTALL_DIR>/bin/wayland-scanner${EXECUTABLE_SUFFIX})

    ExternalProject_Get_Property(wayland-scanner.cross install_dir)


    set(WAYLAND_SCANNER_EXECUTABLE  ${install_dir}/bin/wayland-scanner${EXECUTABLE_SUFFIX})
    add_executable(wayland::scanner-cross IMPORTED GLOBAL)
    set_target_properties(wayland::scanner-cross PROPERTIES
        IMPORTED_LOCATION ${WAYLAND_SCANNER_EXECUTABLE})
    add_dependencies(wayland::scanner-cross wayland-scanner.cross)
else()
    add_executable(wayland::scanner-cross IMPORTED GLOBAL)
    set_property(TARGET wayland::scanner-cross PROPERTY IMPORTED_LOCATION
        ${WAYLAND_SCANNER_EXECUTABLE})
endif()
