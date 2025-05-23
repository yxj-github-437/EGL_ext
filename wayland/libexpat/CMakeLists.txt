cmake_minimum_required(VERSION 3.11)

project(expat VERSION 2.7.1)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

set(expat_version ${PROJECT_VERSION})
string(REPLACE "." "_" R_version ${expat_version})
FetchContent_Declare(
    expat
    URL https://github.com/libexpat/libexpat/releases/download/R_${R_version}/expat-${expat_version}.tar.xz
    URL_HASH SHA256=354552544b8f99012e5062f7d570ec77f14b412a3ff5c7d8d0dae62c0d217c30)
set(EXPAT_SHARED_LIBS OFF CACHE INTERNAL "")
set(EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "")
set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "")
set(EXPAT_WITH_LIBBSD OFF CACHE INTERNAL "")
set(EXPAT_ENABLE_INSTALL OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(expat)
