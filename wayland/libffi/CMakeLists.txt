cmake_minimum_required(VERSION 3.11)

project(libffi)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

set(ffi_version 3.4.7)
FetchContent_Declare(
    libffi
    URL https://github.com/libffi/libffi/releases/download/v${ffi_version}/libffi-${ffi_version}.tar.gz
    URL_HASH SHA256=138607dee268bdecf374adf9144c00e839e38541f75f24a1fcf18b78fda48b2d
    PATCH_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.cmake <SOURCE_DIR>/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/fficonfig_cmake.h.in <SOURCE_DIR>/)
FetchContent_MakeAvailable(libffi)
