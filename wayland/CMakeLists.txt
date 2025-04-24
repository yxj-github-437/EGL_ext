cmake_minimum_required(VERSION 3.11)
project(wayland)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
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
set(wayland_version 1.23.1)
FetchContent_Declare(
    wayland
    URL https://gitlab.freedesktop.org/wayland/wayland/-/releases/${wayland_version}/downloads/wayland-${wayland_version}.tar.xz
    URL_HASH SHA256=864fb2a8399e2d0ec39d56e9d9b753c093775beadc6022ce81f441929a81e5ed
    PATCH_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/CMakeLists.cmake <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/protocol/CMakeLists.cmake <SOURCE_DIR>/protocol/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/config.h.in <SOURCE_DIR>/config.h.in
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/host/ <SOURCE_DIR>/host/
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/patches/0001-fix-undefined-symbols-before-android24.patch
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/patches/0001-use-android-liblog.patch
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland-cmake/patches/0001-use-TMPDIR-when-XDG_RUNTIME_DIR-is-unset-in-android.patch)
FetchContent_MakeAvailable(wayland)
