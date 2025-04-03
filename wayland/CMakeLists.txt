cmake_minimum_required(VERSION 3.11)
project(wayland)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

find_program(Git NAMES git)
find_program(Patch NAMES patch)

if(Git)
    message(STATUS "find Git ${Git}")
    set(patch_command ${Git} apply)
elseif(Patch)
    message(STATUS "find Patch ${Patch}")
    set(patch_command ${Patch} -p1 -i)
else()
    message(FATAL_ERROR "cannot find Git or Patch")
endif()

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
