cmake_minimum_required(VERSION 3.11)

project(libxml2 VERSION 2.14.3)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

set(libxml2_version ${PROJECT_VERSION})
FetchContent_Declare(
    libxml2
    URL https://github.com/GNOME/libxml2/archive/refs/tags/v${libxml2_version}.tar.gz
    URL_HASH SHA256=e282bfeb750be5f4baf98195c6393f9b7beadc6bed4d85e7ca37a66f56394b5f)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(LIBXML2_WITH_ICONV OFF CACHE INTERNAL "")
set(LIBXML2_WITH_DEBUG OFF CACHE INTERNAL "")
set(LIBXML2_WITH_LZMA OFF CACHE INTERNAL "")
set(LIBXML2_WITH_ZLIB OFF CACHE INTERNAL "")
set(LIBXML2_WITH_PROGRAMS OFF CACHE INTERNAL "")
set(LIBXML2_WITH_TESTS OFF CACHE INTERNAL "")
set(LIBXML2_WITH_PYTHON OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(libxml2)
