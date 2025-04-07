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
        set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} ${PATCH_SEARCH_DIR})
        unset(PATCH_SEARCH_DIR)
        unset(GIT_DIR)
    endif()
endif()
find_package(Patch REQUIRED)

set(patch_command ${Patch_EXECUTABLE} -p1 -i)

set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/${CMAKE_HOST_SYSTEM_NAME})

include(FetchContent)
set(wayland_version 1.23.0)
FetchContent_Declare(
    wayland
    URL https://gitlab.freedesktop.org/wayland/wayland/-/releases/${wayland_version}/downloads/wayland-${wayland_version}.tar.xz
    URL_HASH SHA256=05b3e1574d3e67626b5974f862f36b5b427c7ceeb965cb36a4e6c2d342e45ab2
    PATCH_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wayland/CMakeLists.cmake <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wayland/protocol/CMakeLists.cmake <SOURCE_DIR>/protocol/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wayland/config.h.in <SOURCE_DIR>/
    COMMAND ${CMAKE_COMMAND} -E make_directory <SOURCE_DIR>/third_party/
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/libffi <SOURCE_DIR>/third_party/libffi
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland/patches/0001-fix-undefined-symbols-before-android24.patch
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland/patches/0001-use-android-liblog.patch
    COMMAND ${patch_command} ${CMAKE_CURRENT_SOURCE_DIR}/wayland/patches/0001-use-TMPDIR-when-XDG_RUNTIME_DIR-is-unset-in-android.patch)
FetchContent_MakeAvailable(wayland)
