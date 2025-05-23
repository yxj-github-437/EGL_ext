cmake_minimum_required(VERSION 3.11)

project(libffi VERSION 3.4.8)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if (POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

include(FetchContent)

set(ffi_version ${PROJECT_VERSION})
FetchContent_Declare(
    libffi
    URL https://github.com/libffi/libffi/releases/download/v${ffi_version}/libffi-${ffi_version}.tar.gz
    URL_HASH SHA256=bc9842a18898bfacb0ed1252c4febcc7e78fa139fd27fdc7a3e30d9d9356119b)

FetchContent_Populate(libffi)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.cmake ${libffi_SOURCE_DIR}/CMakeLists.txt COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fficonfig.h.cmake ${libffi_SOURCE_DIR}/fficonfig.h.cmake COPYONLY)
add_subdirectory(${libffi_SOURCE_DIR} ${libffi_BINARY_DIR})
