cmake_minimum_required(VERSION ${CMAKE_VERSION})
project(native-wayland-scanner)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

add_subdirectory(libexpat)

add_executable(wayland-scanner
        wayland-util.c
        scanner.c)
target_link_libraries(wayland-scanner
        expat)
if (ANDROID)
    target_link_libraries(wayland-scanner log)
endif()

install(TARGETS wayland-scanner
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
