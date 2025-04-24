cmake_minimum_required(VERSION ${CMAKE_VERSION})

enable_language(ASM)

option(FFI_SHARED_LIB "Build shared library" ON)
option(FFI_STATIC_LIB "Build static library" ON)

include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/configure.ac
        VERSION_CONTENT REGEX "AC_INIT")
string(REGEX MATCHALL "[0-9]+" VERSION_LIST ${VERSION_CONTENT})
list(LENGTH VERSION_LIST LIST_LEN)

if(LIST_LEN EQUAL 2)
    list(GET VERSION_LIST 0 VERSION_0)
    list(GET VERSION_LIST 1 VERSION_1)
    set(VERSION ${VERSION_0}.${VERSION_1})
elseif(LIST_LEN EQUAL 3)
    list(GET VERSION_LIST 0 VERSION_0)
    list(GET VERSION_LIST 1 VERSION_1)
    list(GET VERSION_LIST 2 VERSION_2)
    set(VERSION ${VERSION_0}.${VERSION_1}.${VERSION_2})
endif()

message(STATUS "libffi version: ${VERSION}")

file(WRITE ${PROJECT_BINARY_DIR}/conftest.h
    "#if defined(__aarch64__)
     CMAKE-ARCH-DETECT: ARM64
     #elif defined(__arm__)
     CMAKE-ARCH-DETECT: ARM
     #elif defined(__i386__)
     CMAKE-ARCH-DETECT: X86
     #elif defined(__x86_64__)
     CMAKE-ARCH-DETECT: X86_64
     #endif")

if(CMAKE_C_COMPILER_TARGET)
    set(FFI_ARCH_TEST_FLAGS "--target=${CMAKE_C_COMPILER_TARGET}")
endif()

execute_process(
    COMMAND
    ${CMAKE_C_COMPILER}
    ${FFI_ARCH_TEST_FLAGS}
    -E ${PROJECT_BINARY_DIR}/conftest.h
    -o -
    OUTPUT_VARIABLE _arch_out
    ERROR_VARIABLE _arch_err
    RESULT_VARIABLE _arch_res
    ERROR_STRIP_TRAILING_WHITESPACE)

if(_arch_res EQUAL 0)
    string(REGEX REPLACE ".*CMAKE-ARCH-DETECT: (.+)\n.*" "\\1" TARGET ${_arch_out})
endif()

file(REMOVE ${PROJECT_BINARY_DIR}/conftest.h)

if(NOT TARGET)
    message(FATAL_ERROR "cannot detect target arch.")
endif()

set(PACKAGE libffi)
set(PACKAGE_BUGREPORT https://github.com/libffi/libffi/issues)
set(PACKAGE_NAME ${PACKAGE})
set(PACKAGE_STRING "${PACKAGE} ${VERSION}")
set(PACKAGE_TARNAME ${PACKAGE})
set(PACKAGE_URL https://github.com/libffi/libffi)
set(PACKAGE_VERSION ${VERSION})
set(LT_OBJDIR .libs/)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    string(REPLACE "/" "\\" CMAKE_READELF ${CMAKE_READELF})
    string(REPLACE "/" "\\" CMAKE_NM ${CMAKE_NM})
endif()

set(SOURCES_LIST
    src/closures.c
    src/java_raw_api.c
    src/prep_cif.c
    src/raw_api.c
    src/tramp.c
    src/types.c)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND SOURCES_LIST src/debug.c)
    set(FFI_DEBUG ON)
endif()

check_type_size(size_t SIZEOF_SIZE_T)

if(SIZEOF_SIZE_T STREQUAL "")
    set(size_t "unsinged int")
endif()

check_include_file(sys/memfd.h HAVE_SYS_MEMFD_H)
check_function_exists(memfd_create HAVE_MEMFD_CREATE)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(mkostemp HAVE_MKOSTEMP)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(memcpy HAVE_MEMCPY)
check_type_size(double SIZEOF_DOUBLE)
check_type_size("long double" SIZEOF_LONG_DOUBLE)

if(SIZEOF_LONG_DOUBLE STREQUAL "")
    set(HAVE_LONG_DOUBLE 0)

    if(DEFINED HAVE_LONG_DOUBLE_VARIANT)
        set(HAVE_LONG_DOUBLE 1)
    elseif(NOT SIZEOF_DOUBLE EQUAL SIZEOF_LONG_DOUBLE)
        set(HAVE_LONG_DOUBLE 1)
    endif()
else()
    set(HAVE_LONG_DOUBLE 1)
endif()

check_symbol_exists(MAP_ANON sys/mman.h HAVE_MMAP_ANON)
set(HAVE_MMAP_DEV_ZERO 1)
check_include_file(alloca.h HAVE_ALLOCA_H)
check_c_source_compiles("
    #include <alloca.h>
    int main()
    {
        char* x = alloca(1024);
        return 0;
    }" HAVE_ALLOCA)

check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
set(LIBFFI_GNU_SYMBOL_VERSIONING 1)
set(FFI_EXEC_TRAMPOLINE_TABLE 0)
set(FFI_MMAP_EXEC_WRIT 0)

if(ANDROID)
    set(FFI_MMAP_EXEC_WRIT 1)
endif()

set(USING_PURIFY 1)
set(STACK_DIRECTION 0)
set(HAVE_MMAP_FILE 1)

option(FFI_NO_RAW_API "Define this if you do not want support for the raw API." OFF)
option(FFI_EXEC_STATIC_TRAMP "Define this if you want statically defined trampolines." ON)

check_c_source_compiles("
    asm (\".cfi_sections\n\t.cfi_startproc\n\t.cfi_endproc\");
    int
    main (void)
    {
        return 0;
    }" HAVE_AS_CFI_PSEUDO_OP)

check_c_source_compiles("
    int
    main (void)
    {
        asm (\".register %g2, #scratch\");
        return 0;
    }" HAVE_AS_REGISTER_PSEUDO_OP)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    set(CMD cmd.exe /C)
else()
    set(CMD /bin/sh -c)
endif()

file(WRITE ${PROJECT_BINARY_DIR}/conftest.c
    "void foo (void) {}; void bar (void) { foo (); foo (); }")
try_compile(COMPILE_RESULT
    ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_BINARY_DIR}/conftest.c
    COMPILE_DEFINITIONS -fPIC -fno-exceptions
    LINK_OPTIONS -shared
    COPY_FILE ${PROJECT_BINARY_DIR}/conftest)
execute_process(
    COMMAND ${CMD} "${CMAKE_READELF} -WS conftest"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    OUTPUT_VARIABLE CMD_OUTPUT
    ERROR_VARIABLE IGNORE
    RESULT_VARIABLE IGNORE)
file(REMOVE ${PROJECT_BINARY_DIR}/conftest.c
        ${PROJECT_BINARY_DIR}/conftest)

if(${CMD_OUTPUT} MATCHES "eh_frame [a-zA-Z0-9 ]* WA")
    set(EH_FRAME_FLAGS "aw")
    message(STATUS "Checking if .eh_frame section is read-only - no")
else()
    set(HAVE_RO_EH_FRAME 1)
    set(EH_FRAME_FLAGS "a")
    message(STATUS "Checking if .eh_frame section is read-only - yes")
endif()

if("${TARGET}" MATCHES "X86*")
    file(WRITE ${PROJECT_BINARY_DIR}/conftest.S
            ".text; foo: nop; .data; .long foo-.; .text")
    try_compile(HAVE_AS_X86_PCREL
            ${PROJECT_BINARY_DIR}
            SOURCES ${PROJECT_BINARY_DIR}/conftest.S
            LINK_OPTIONS -shared)
    file(REMOVE ${PROJECT_BINARY_DIR}/conftest.S)

    if(HAVE_AS_X86_PCREL)
        set(HAVE_AS_X86_PCREL 1)
        message(STATUS "Checking HAVE_AS_X86_PCREL - yes")
    else()
        message(STATUS "Checking HAVE_AS_X86_PCREL - no")
    endif()
endif()

if("${TARGET}" STREQUAL "X86_64")
    file(WRITE ${PROJECT_BINARY_DIR}/conftest.S
            ".text;.globl foo;foo:;jmp bar;.section .eh_frame,\"a\",@unwind;bar:")
    file(WRITE ${PROJECT_BINARY_DIR}/conftest.c
            "extern void foo();int main(){foo();}")

    try_compile(HAVE_AS_X86_64_UNWIND_SECTION_TYPE
        ${PROJECT_BINARY_DIR}
        SOURCES
            ${PROJECT_BINARY_DIR}/conftest.S
            ${PROJECT_BINARY_DIR}/conftest.c
        COMPILE_DEFINITIONS -Wa,--fatal-warnings)
    file(REMOVE ${PROJECT_BINARY_DIR}/conftest.c
        ${PROJECT_BINARY_DIR}/conftest.S)

    if(HAVE_AS_X86_64_UNWIND_SECTION_TYPE)
        set(HAVE_AS_X86_64_UNWIND_SECTION_TYPE 1)
        message(STATUS "Checking HAVE_AS_X86_64_UNWIND_SECTION_TYPE - yes")
    else()
        message(STATUS "Checking HAVE_AS_X86_64_UNWIND_SECTION_TYPE - no")
    endif()
endif()

file(WRITE ${PROJECT_BINARY_DIR}/conftest.c
        "void nm_test_func(){} int main(){nm_test_func();return 0;}")
try_compile(COMPILE_RESULT
        ${PROJECT_BINARY_DIR}
        SOURCES ${PROJECT_BINARY_DIR}/conftest.c
        COPY_FILE ${PROJECT_BINARY_DIR}/conftest)

execute_process(
    COMMAND ${CMD} "${CMAKE_NM} -a conftest"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    OUTPUT_VARIABLE CMD_OUTPUT
    ERROR_VARIABLE IGNORE
    RESULT_VARIABLE IGNORE)

file(REMOVE ${PROJECT_BINARY_DIR}/conftest.c
        ${PROJECT_BINARY_DIR}/conftest)

if(${CMD_OUTPUT} MATCHES "_nm_test_func")
    set(SYMBOL_UNDERSCORE 1)
    message(STATUS "Checking if symbols are underscored - yes")
else()
    message(STATUS "Checking if symbols are underscored - no")
endif()

file(WRITE ${PROJECT_BINARY_DIR}/conftest.c
        "int __attribute__ ((visibility (\"hidden\"))) foo(void){return 1;}")
try_compile(COMPILE_RESULT
        ${PROJECT_BINARY_DIR}
        SOURCES ${PROJECT_BINARY_DIR}/conftest.c
        COMPILE_DEFINITIONS -Werror
        LINK_OPTIONS -shared
        COPY_FILE ${PROJECT_BINARY_DIR}/conftest)
execute_process(
    COMMAND ${CMD} "${CMAKE_READELF} -a conftest"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT_VARIABLE CMD_OUTPUT
    ERROR_VARIABLE IGNORE
    RESULT_VARIABLE IGNORE)
file(REMOVE ${PROJECT_BINARY_DIR}/conftest.c
    ${PROJECT_BINARY_DIR}/conftest)

if(${CMD_OUTPUT} MATCHES "HIDDEN [0-9 ]* foo")
    set(HAVE_HIDDEN_VISIBILITY_ATTRIBUTE 1)
    message(STATUS "Checking HAVE_HIDDEN_VISIBILITY_ATTRIBUTE - yes")
else()
    message(STATUS "Checking HAVE_HIDDEN_VISIBILITY_ATTRIBUTE - no")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ffi.h.in
    ${PROJECT_BINARY_DIR}/include/ffi.h)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/fficonfig.h.cmake
    ${PROJECT_BINARY_DIR}/include/fficonfig.h)

if("${TARGET}" STREQUAL "ARM64")
    set(TARGETDIR aarch64)
    list(APPEND SOURCES_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/aarch64/sysv.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/aarch64/ffi.c)
elseif("${TARGET}" STREQUAL "ARM")
    set(TARGETDIR arm)
    list(APPEND SOURCES_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arm/sysv.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arm/ffi.c)
elseif("${TARGET}" STREQUAL "X86")
    set(TARGETDIR x86)
    list(APPEND SOURCES_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/x86/sysv.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/x86/ffi.c)
elseif("${TARGET}" STREQUAL "X86_64")
    set(TARGETDIR x86)
    list(APPEND SOURCES_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/x86/unix64.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/x86/win64.S
        ${CMAKE_CURRENT_SOURCE_DIR}/src/x86/ffiw64.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/x86/ffi64.c)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/${TARGETDIR}/ffitarget.h
    ${PROJECT_BINARY_DIR}/include/ffitarget.h COPYONLY)

add_library(ffi_core OBJECT ${SOURCES_LIST})
target_compile_options(ffi_core PRIVATE -Wno-deprecated-declarations)
target_include_directories(ffi_core
    PUBLIC
        ${PROJECT_BINARY_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(FFI_STATIC_LIB)
    add_library(ffi_static STATIC $<TARGET_OBJECTS:ffi_core>)
    target_include_directories(ffi_static
        PUBLIC
        ${PROJECT_BINARY_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(ffi_static PROPERTIES OUTPUT_NAME ffi
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

if(FFI_SHARED_LIB)
    add_library(ffi_shared SHARED $<TARGET_OBJECTS:ffi_core>)
    target_include_directories(ffi_shared
        PUBLIC
        ${PROJECT_BINARY_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(ffi_shared PROPERTIES OUTPUT_NAME ffi
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
