/* fficonfig.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
#cmakedefine AC_APPLE_UNIVERSAL_BUILD

/* Define to the flags needed for the .section .eh_frame directive. */
#cmakedefine EH_FRAME_FLAGS "@EH_FRAME_FLAGS@"

/* Define this if you want extra debugging. */
#cmakedefine FFI_DEBUG

/* Define this if you want statically defined trampolines */
#cmakedefine FFI_EXEC_STATIC_TRAMP

/* Cannot use PROT_EXEC on this target, so, we revert to alternative means */
#cmakedefine FFI_EXEC_TRAMPOLINE_TABLE

/* Define this if you want to enable pax emulated trampolines (experimental)
 */
#cmakedefine01 FFI_MMAP_EXEC_EMUTRAMP_PAX

/* Cannot use malloc on this target, so, we revert to alternative means */
#cmakedefine01 FFI_MMAP_EXEC_WRIT

/* Define this if you do not want support for the raw API. */
#cmakedefine01 FFI_NO_RAW_API

/* Define this if you do not want support for aggregate types. */
#cmakedefine01 FFI_NO_STRUCTS

/* Define to 1 if <alloca.h> works. */
#cmakedefine01 HAVE_ALLOCA_H

/* Define if your compiler supports pointer authentication. */
#cmakedefine HAVE_ARM64E_PTRAUTH

/* Define if your assembler supports .cfi_* directives. */
#cmakedefine HAVE_AS_CFI_PSEUDO_OP

/* Define if your assembler supports .register. */
#cmakedefine HAVE_AS_REGISTER_PSEUDO_OP

/* Define if the compiler uses zarch features. */
#cmakedefine HAVE_AS_S390_ZARCH

/* Define if your assembler and linker support unaligned PC relative relocs.
 */
#cmakedefine HAVE_AS_SPARC_UA_PCREL

/* Define if your assembler supports unwind section type. */
#cmakedefine HAVE_AS_X86_64_UNWIND_SECTION_TYPE

/* Define if your assembler supports PC relative relocs. */
#cmakedefine HAVE_AS_X86_PCREL

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine01 HAVE_DLFCN_H

/* Define if __attribute__((visibility("hidden"))) is supported. */
#cmakedefine HAVE_HIDDEN_VISIBILITY_ATTRIBUTE

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine01 HAVE_INTTYPES_H

/* Define if you have the long double type and it is bigger than a double */
#cmakedefine01 HAVE_LONG_DOUBLE

/* Define if you support more than one size of the long double type */
#cmakedefine HAVE_LONG_DOUBLE_VARIANT

/* Define to 1 if you have the `memcpy' function. */
#cmakedefine01 HAVE_MEMCPY

/* Define to 1 if you have the `memfd_create' function. */
#cmakedefine HAVE_MEMFD_CREATE

/* Define if .eh_frame sections should be read-only. */
#cmakedefine HAVE_RO_EH_FRAME

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine01 HAVE_STDINT_H

/* Define to 1 if you have the <stdio.h> header file. */
#cmakedefine01 HAVE_STDIO_H

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine01 HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine01 HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine01 HAVE_STRING_H

/* Define to 1 if you have the <sys/memfd.h> header file. */
#cmakedefine01 HAVE_SYS_MEMFD_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine01 HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine01 HAVE_SYS_TYPES_H

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine01 HAVE_UNISTD_H

/* Define to 1 if GNU symbol versioning is used for libatomic. */
#cmakedefine01 LIBFFI_GNU_SYMBOL_VERSIONING

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#cmakedefine LT_OBJDIR "@LT_OBJDIR@"

/* Name of package */
#cmakedefine PACKAGE "@PACKAGE@"

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PACKAGE_STRING@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PACKAGE_TARNAME@"

/* Define to the home page for this package. */
#cmakedefine PACKAGE_URL "@PACKAGE_URL@"

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

/* The size of `double', as computed by sizeof. */
#cmakedefine SIZEOF_DOUBLE @SIZEOF_DOUBLE@

/* The size of `long double', as computed by sizeof. */
#cmakedefine SIZEOF_LONG_DOUBLE @SIZEOF_LONG_DOUBLE@

/* The size of `size_t', as computed by sizeof. */
#cmakedefine SIZEOF_SIZE_T @SIZEOF_SIZE_T@

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#cmakedefine01 STDC_HEADERS

/* Define if symbols are underscored. */
#cmakedefine SYMBOL_UNDERSCORE

/* Define this if you are using Purify and want to suppress spurious messages.
 */
#cmakedefine USING_PURIFY

/* Version number of package */
#cmakedefine VERSION "@VERSION@"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
#if defined __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#endif
#else
#ifndef WORDS_BIGENDIAN
#undef WORDS_BIGENDIAN
#endif
#endif

#ifdef HAVE_HIDDEN_VISIBILITY_ATTRIBUTE
#ifdef LIBFFI_ASM
#ifdef __APPLE__
#define FFI_HIDDEN(name) .private_extern name
#else
#define FFI_HIDDEN(name) .hidden name
#endif
#else
#define FFI_HIDDEN __attribute__((visibility("hidden")))
#endif
#else
#ifdef LIBFFI_ASM
#define FFI_HIDDEN(name)
#else
#define FFI_HIDDEN
#endif
#endif
