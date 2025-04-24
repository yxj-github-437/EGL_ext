cmake_minimum_required(VERSION 3.11)

project(expat)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
    expat
    URL https://github.com/libexpat/libexpat/releases/download/R_2_6_4/expat-2.6.4.tar.xz
    URL_HASH SHA256=a695629dae047055b37d50a0ff4776d1d45d0a4c842cf4ccee158441f55ff7ee)
set(EXPAT_SHARED_LIBS OFF CACHE INTERNAL "")
set(EXPAT_BUILD_TOOLS OFF CACHE INTERNAL "")
set(EXPAT_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(EXPAT_BUILD_TESTS OFF CACHE INTERNAL "")
set(EXPAT_WITH_LIBBSD OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(expat)
