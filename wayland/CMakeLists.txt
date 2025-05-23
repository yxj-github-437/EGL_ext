cmake_minimum_required(VERSION 3.11)
project(wayland VERSION 1.23.91)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if (POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

if(CMAKE_HOST_WIN32)
    find_package(Git)

    if(Git_FOUND)
        get_filename_component(GIT_DIR ${GIT_EXECUTABLE} DIRECTORY)
        get_filename_component(PATCH_SEARCH_DIR ${GIT_DIR}/../usr/bin REALPATH)
        list(APPEND CMAKE_PROGRAM_PATH ${PATCH_SEARCH_DIR})
        unset(PATCH_SEARCH_DIR)
        unset(GIT_DIR)
    endif()
endif()

find_package(Patch REQUIRED)
set(patch_command ${Patch_EXECUTABLE} -p1 -i)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

option(WAYLAND_BUILD_SCANNER "Compile wayland-scanner binary" ON)
option(WAYLAND_BUILD_LIBRARIES "Compile Wayland libraries" ON)
option(WAYLAND_BUILD_SERVER "Build Server bindings" ON)
option(WAYLAND_BUILD_CLIENT "Build Client bindings" ON)

if (NOT UNIX)
    set(WAYLAND_BUILD_LIBRARIES OFF)
endif()

if (WAYLAND_BUILD_LIBRARIES)
    set(FFI_STATIC_LIB ON)
    set(FFI_SHARED_LIB OFF)
    add_subdirectory(libffi)
endif()

if (WAYLAND_BUILD_SCANNER)
    add_subdirectory(libexpat)
    find_package(Python3 COMPONENTS Interpreter Development)
    if (Python3_EXECUTABLE)
        add_subdirectory(libxml2)
    endif()
endif()

include(FetchContent)
set(wayland_version ${PROJECT_VERSION})
FetchContent_Declare(
    wayland
    URL https://gitlab.freedesktop.org/wayland/wayland/-/releases/${wayland_version}/downloads/wayland-${wayland_version}.tar.xz
    URL_HASH SHA256=fc2bec1a53e24f48b83d17e72d6d1170239d31edc3f3c9383b4ac2188349a304
    PATCH_COMMAND
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/host <SOURCE_DIR>/host
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/patches/0001-fix-undefined-symbols-before-android24.patch
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/patches/0001-use-android-liblog.patch
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/patches/0001-use-TMPDIR-when-XDG_RUNTIME_DIR-is-unset-in-android.patch)

FetchContent_Populate(wayland)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/CMakeLists.cmake ${wayland_SOURCE_DIR}/CMakeLists.txt COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/protocol/CMakeLists.cmake ${wayland_SOURCE_DIR}/protocol/CMakeLists.txt COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/config.h.in ${wayland_SOURCE_DIR}/config.h.in COPYONLY)
add_subdirectory(${wayland_SOURCE_DIR} ${wayland_BINARY_DIR})


message(STATUS "===========================================================================")
message(STATUS "")
message(STATUS "Build wayland-scanner       ${WAYLAND_BUILD_SCANNER}")
message(STATUS "Build Wayland libraries     ${WAYLAND_BUILD_LIBRARIES}")

if(WAYLAND_BUILD_LIBRARIES)
    message(STATUS "Build wayland-server        ${WAYLAND_BUILD_LIBRARIES}")
    message(STATUS "Build wayland-client        ${WAYLAND_BUILD_CLIENT}")
endif()

message(STATUS "")
message(STATUS "===========================================================================")

if(WAYLAND_BUILD_SCANNER)
    install(TARGETS wayland-scanner
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(WAYLAND_BUILD_LIBRARIES)
    install(DIRECTORY ${PROJECT_BINARY_DIR}/include
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING PATTERN "*.h")

    if(WAYLAND_BUILD_SERVER)
        install(TARGETS wayland-server
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()

    if(WAYLAND_BUILD_CLIENT)
        install(TARGETS wayland-client wayland-egl wayland-cursor
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
endif()

if(NOT TARGET uninstall)
    configure_file(${PROJECT_SOURCE_DIR}/wayland-cmake/cmake_uninstall.cmake.in
        ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake)
endif()
