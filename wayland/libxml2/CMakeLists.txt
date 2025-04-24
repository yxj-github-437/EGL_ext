cmake_minimum_required(VERSION 3.11)

project(libxml2)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(libxml2_version 2.13.5)
FetchContent_Declare(
    libxml2
    URL https://github.com/GNOME/libxml2/archive/refs/tags/v${libxml2_version}.tar.gz
    URL_HASH SHA256=87d681718760ba42e749a61482d71b34d1d566843e9ade39a73fea92caf8293b)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(LIBXML2_WITH_ICONV OFF CACHE INTERNAL "")
set(LIBXML2_WITH_DEBUG OFF CACHE INTERNAL "")
set(LIBXML2_WITH_LZMA OFF CACHE INTERNAL "")
set(LIBXML2_WITH_ZLIB OFF CACHE INTERNAL "")
set(LIBXML2_WITH_PROGRAMS OFF CACHE INTERNAL "")
set(LIBXML2_WITH_TESTS OFF CACHE INTERNAL "")
set(LIBXML2_WITH_PYTHON OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(libxml2)
