#ifndef LIBZIP_CONFIG_H
#define LIBZIP_CONFIG_H

#include "../config.h"

/* Definitions shared between Linux, Mac OS X, and Windows. */
#define HAVE_GETOPT 1
#define HAVE_INT16_T 1
#define HAVE_INT32_T 1
#define HAVE_INT64_T 1
#define HAVE_INT8_T 1
#define HAVE_OPEN 1
#define HAVE_SSIZE_T 1
#define HAVE_UINT16_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT64_T 1
#define HAVE_UINT8_T 1

/* Definitions shared between Linux and Mac OS X. */
#ifdef __unix__
  #define HAVE_DLFCN_H 1
  #define HAVE_FSEEKO 1
  #define HAVE_FTELLO 1
  #define HAVE_LIBZ 1
  #define HAVE_MKSTEMP 1
  #define HAVE_STRUCT_TM_TM_ZONE 1
  #define HAVE_TM_ZONE 1
#endif

/* Definitions specific to Windows. */
#ifdef _WIN32
  #define HAVE_DECL_TZNAME 1
  #define HAVE_MOVEFILEEXA 1
  #define HAVE_TZNAME 1
  #define HAVE__CLOSE 1
  #define HAVE__DUP 1
  #define HAVE__OPEN 1
  #define HAVE__SNPRINTF 1
  #define HAVE__STRDUP 1
  #define HAVE__STRICMP 1
#endif

/* Define variable sizes. */
#ifdef __SIZEOF_INT__
  #define SIZEOF_INT __SIZEOF_INT__
#endif
#ifdef __SIZEOF_LONG__
  #define SIZEOF_LONG __SIZEOF_LONG__
#endif
#ifdef __SIZEOF_LONG_LONG__
  #define SIZEOF_LONG_LONG __SIZEOF_LONG_LONG__
#endif
#ifdef __SIZEOF_SIZE_T__
  #define SIZEOF_OFF_T __SIZEOF_SIZE_T__
  #define SIZEOF_SIZE_T __SIZEOF_SIZE_T__
#endif
#ifdef __SIZEOF_SHORT__
  #define SIZEOF_SHORT __SIZEOF_SHORT__
#endif

#ifdef __x86_64
  #ifndef SIZEOF_INT
    #define SIZEOF_INT 4
  #endif
  #ifndef SIZEOF_LONG
    #define SIZEOF_LONG 8
  #endif
  #ifndef SIZEOF_LONG_LONG
    #define SIZEOF_LONG_LONG 8
  #endif
  #ifndef SIZEOF_OFF_T
    #define SIZEOF_OFF_T 8
  #endif
  #ifndef SIZEOF_SHORT
    #define SIZEOF_SHORT 2
  #endif
  #ifndef SIZEOF_SIZE_T
    #define SIZEOF_SIZE_T 8
  #endif
#elif defined __i386
  #ifndef SIZEOF_INT
    #define SIZEOF_INT 4
  #endif
  #ifndef SIZEOF_LONG
    #define SIZEOF_LONG 4
  #endif
  #ifndef SIZEOF_LONG_LONG
    #define SIZEOF_LONG_LONG 8
  #endif
  #ifndef SIZEOF_OFF_T
    #define SIZEOF_OFF_T 4
  #endif
  #ifndef SIZEOF_SHORT
    #define SIZEOF_SHORT 2
  #endif
  #ifndef SIZEOF_SIZE_T
    #define SIZEOF_SIZE_T 4
  #endif
#endif

#endif
