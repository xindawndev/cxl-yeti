#ifndef _CXL_YETI_CONFIG_H_
#define _CXL_YETI_CONFIG_H_

/*----------------------------------------------------------------------
|   命名空间定义
+---------------------------------------------------------------------*/
#define MYNAME1_        cxl
#define MYNAME2_        yeti
#define NAMEBEG_(_x)    namespace _x {
#define NAMEBEG1        NAMEBEG_(MYNAME1_)
#define NAMEBEG2        NAMEBEG_(MYNAME2_)
#define NAMEBEG         NAMEBEG1 NAMEBEG2
#define NAMEEND1        }
#define NAMEEND         NAMEEND1 NAMEEND1

#define USINGNAMESPACE1          using namespace MYNAME1_
#define USINGNAMESPACE2          using namespace MYNAME1_::MYNAME2_

/*----------------------------------------------------------------------
|   所需包含头文件
+---------------------------------------------------------------------*/
#define YETI_CONFIG_HAVE_ASSERT_H       // assert.h
#define YETI_CONFIG_HAVE_STD_C          // stdc
#define YETI_CONFIG_HAVE_POSIX_TIME     // posix time
#define YETI_CONFIG_HAVE_STDLIB_H       // stdlib.h
#define YETI_CONFIG_HAVE_STDIO_H        // stdio.h
#define YETI_CONFIG_HAVE_STDARG_H       // stdarg.h
#define YETI_CONFIG_HAVE_STDINT_H       // stdint.h
#define YETI_CONFIG_HAVE_STRING_H       // string.h
#define YETI_CONFIG_HAVE_LIMITS_H       // limits.h

/*----------------------------------------------------------------------
|   标准C运行时库
+---------------------------------------------------------------------*/
#if defined(YETI_CONFIG_HAVE_STD_C)
#   define YETI_CONFIG_HAVE_MALLOC         // malloc();
#   define YETI_CONFIG_HAVE_CALLOC         // calloc();
#   define YETI_CONFIG_HAVE_REALLOC        // realloc();
#   define YETI_CONFIG_HAVE_FREE           // free();
#   define YETI_CONFIG_HAVE_MEMCPY         // memcpy();
#   define YETI_CONFIG_HAVE_MEMSET         // memset();
#   define YETI_CONFIG_HAVE_MEMCMP         // memcmp();
#   define YETI_CONFIG_HAVE_GETENV         // getenv();
#   define YETI_CONFIG_HAVE_SETENV         // setenv();
#   define YETI_CONFIG_HAVE_UNSETENV       // unsetenv();
#   define YETI_CONFIG_HAVE_READDIR_R      // readdir_r();
#endif /* YETI_CONFIG_HAVE_STD_C */

#if defined(YETI_CONFIG_HAVE_POSIX_TIME)
#   define YETI_CONFIG_HAVE_GMTIME         // gmtime();
#   define YETI_CONFIG_HAVE_GMTIME_R       // gmtime_r();
#   define YETI_CONFIG_HAVE_LOCALTIME      // localtime();
#   define YETI_CONFIG_HAVE_LOCALTIME_R    // localtime_r();
#endif /* YETI_CONFIG_HAVE_POSIX_TIME */

#if defined(YETI_CONFIG_HAVE_STRING_H)
#   define YETI_CONFIG_HAVE_STRCMP             // strcmp();
#   define YETI_CONFIG_HAVE_STRNCMP            // strncmp();
#   define YETI_CONFIG_HAVE_STRDUP             // strdup();
#   define YETI_CONFIG_HAVE_STRLEN             // strlen();
#   define YETI_CONFIG_HAVE_STRCPY             // strcpy();
#   define YETI_CONFIG_HAVE_STRNCPY            // strncpy();
#endif /* YETI_CONFIG_HAVE_STRING_H */

#if defined(YETI_CONFIG_HAVE_STDIO_H)
#   define YETI_CONFIG_HAVE_SPRINTF            // sprintf();
#   define YETI_CONFIG_HAVE_SNPRINTF           // snprintf();
#   define YETI_CONFIG_HAVE_VSPRINTF           // vsprintf();
#   define YETI_CONFIG_HAVE_VSNPRINTF          // vsnprintf();
#endif /* YETI_CONFIG_HAVE_STDIO_H */

#if defined(YETI_CONFIG_HAVE_LIMITS_H)
#   define YETI_CONFIG_HAVE_INT_MIN            // int min();
#   define YETI_CONFIG_HAVE_INT_MAX            // int max();
#   define YETI_CONFIG_HAVE_UINT_MAX           // uint max();
#   define YETI_CONFIG_HAVE_LONG_MIN           // long min();
#   define YETI_CONFIG_HAVE_LONG_MAX           // long max();
#   define YETI_CONFIG_HAVE_ULONG_MAX          // ulong max();
#endif /* YETI_CONFIG_HAVE_LIMITS_H */

/*----------------------------------------------------------------------
|   标准C++运行时库
+---------------------------------------------------------------------*/
#define YETI_CONFIG_HAVE_NEW_H              // new.h

/*----------------------------------------------------------------------
|   sockets
+---------------------------------------------------------------------*/
#define YETI_CONFIG_HAVE_SOCKADDR_SA_LEN    // sockaddr sa len

/*----------------------------------------------------------------------
|   平台特性相关
+---------------------------------------------------------------------*/
/* Windows 32 */
#if defined(_WIN32) || defined(_XBOX)
#   if !defined(STRICT)
#       define STRICT
#   endif
#endif

/* XBox */
#if defined(_XBOX)
#   define YETI_CONFIG_THREAD_STACK_SIZE 0x10000
#endif

/* QNX */
#if defined(__QNX__)
#endif

/* cygwin */
#if defined(__CYGWIN__)
#   undef YETI_CONFIG_HAVE_SOCKADDR_SA_LEN
#endif

/* linux */
#if defined(__linux__)
#   define YETI_CONFIG_HAVE_GETADDRINFO
#   undef YETI_CONFIG_HAVE_SOCKADDR_SA_LEN
#endif

/* symbian */
#if defined(__SYMBIAN32__)
/* If defined, specify the stack size of each YETI_Thread. */
#   define YETE_CONFIG_THREAD_STACK_SIZE   0x14000
#endif

/* android */
#if defined(ANDROID)
#   define YETI_CONFIG_HAVE_GETADDRINFO
#   undef YETI_CONFIG_HAVE_SOCKADDR_SA_LEN
#endif

/* OSX and iOS */
#if defined(__APPLE__)
#   define YETI_CONFIG_HAVE_GETADDRINFO
#   define YETI_CONFIG_HAVE_AUTORELEASE_POOL
//# define YETI_CONFIG_HAVE_SYSTEM_LOG_CONFIG
#endif

/*----------------------------------------------------------------------
|   编译器特性相关
+---------------------------------------------------------------------*/
/* GCC */
#if defined(__GNUC__)
#   define YETI_LocalFunctionName __FUNCTION__
#   define YETI_COMPILER_UNUSED(p) (void)p
#else
#   define YETI_COMPILER_UNUSED(p) 
#endif

/* TriMedia C/C++ Compiler */
#if defined(__TCS__)
#   undef YETI_CONFIG_HAVE_ASSERT_H           // 无assert.h
#   undef YETI_CONFIG_HAVE_SNPRINTF           // 无snprintf();
#   undef YETI_CONFIG_HAVE_VSNPRINTF          // 无vsnprintf();
#endif

/* palmos compiler */
#if defined(__PALMOS__)
#   if __PALMOS__ <= 0x05000000
#       undef YETI_CONFIG_HAVE_ASSERT_H           // 无assert.h
#       undef YETI_CONFIG_HAVE_SNPRINTF           // 无snprintf();
#       undef YETI_CONFIG_HAVE_VSNPRINTF          // 无vsnprintf();
#   endif
#endif

/* Microsoft C/C++ Compiler */
#if defined(_MSC_VER)
#   undef YETI_CONFIG_HAVE_STDINT_H
#   define YETI_CONFIG_HAVE_GETADDRINFO
#   define YETI_CONFIG_STAT_ST_CTIME_IS_ST_BIRTHTIME
#   define YETI_FORMAT_64 "I64"
#   define YETI_CONFIG_INT64_TYPE __int64
#   define YETI_INT64_MIN _I64_MIN
#   define YETI_INT64_MAX _I64_MAX
#   define YETI_UINT64_MAX _UI64_MAX
#   define YETI_INT64_C(_x) _x##i64
#   define YETI_UINT64_C(_x) _x##ui64
#   define YETI_LocalFunctionName __FUNCTION__
#   if !defined(_WIN32_WCE)
#       define YETI_fseek _fseeki64
#       define YETI_ftell _ftelli64
#   else
#       define YETI_fseek(a,b,c) fseek((a),(long)(b), (c))
#       define YETI_ftell ftell
#   endif
#   define YETI_stat  YETI_stat_utf8
#   define YETI_stat_struct struct __stat64
#   if defined(_WIN64)
        typedef __int64 YETI_PointerLong;
#   else
#       if _MSC_VER >= 1400
            typedef __w64 long YETI_PointerLong;
#       else
            typedef long YETI_PointerLong;
#       endif
#   endif
#   define YETI_POINTER_TO_LONG(_p) ((YETI_PointerLong) (_p) )
#   if _MSC_VER >= 1400 && !defined(_WIN32_WCE)
#       define gmtime_r(a,b) gmtime_s(a,b)
#       define localtime_r(a,b) localtime_s(b,a)
#       define YETI_CONFIG_HAVE_FOPEN_S
#       define YETI_CONFIG_HAVE_FSOPEN
#       define YETI_CONFIG_HAVE_SHARE_H
#       define YETI_vsnprintf(s,c,f,a)  _vsnprintf_s(s,c,_TRUNCATE,f,a)
#       define YETI_snprintf(s,c,f,...) _snprintf_s(s,c,_TRUNCATE,f,__VA_ARGS__)
#       define YETI_strncpy(d,s,c)       strncpy_s(d,c+1,s,c)
#       define YETI_strcpy(d,s)          strcpy_s(d,strlen(s)+1,s)
#       undef YETI_CONFIG_HAVE_GETENV
#       define YETI_CONFIG_HAVE_DUPENV_S
#       define dupenv_s _dupenv_s
#       undef YETI_CONFIG_HAVE_SETENV
#       undef YETI_CONFIG_HAVE_UNSETENV
#       define YETI_CONFIG_HAVE_PUTENV_S
#       define putenv_s _putenv_s
#   else
#       undef YETI_CONFIG_HAVE_GMTIME_R
#       undef YETI_CONFIG_HAVE_LOCALTIME_R
#       define YETI_vsnprintf  _vsnprintf
#       define YETI_snprintf   _snprintf
#   endif
#   if defined(_DEBUG)
#       define _CRTDBG_MAP_ALLOC
#   endif
#endif

/* Windows CE */
#if defined(_WIN32_WCE)
#   if defined(YETI_CONFIG_HAVE_FOPEN_S)
#       undef YETI_CONFIG_HAVE_FOPEN_S
#   endif
#endif

/* Symbian */
#if defined(__SYMBIAN32__)
#   undef YETI_CONFIG_HAVE_NEW_H
#   include "e32std.h"
#   define explicit
#   define YETI_fseek fseek  // no fseeko ?
#   define YETI_ftell ftell  // no ftello ?
#endif

/* Android */
#if defined(ANDROID)
#   define YETI_CONFIG_NO_RTTI
#endif

/* OSX and iOS */
#if defined(__APPLE__)
#   include <TargetConditionals.h>
#   include <AvailabilityMacros.h>
#   define YETI_CONFIG_HAVE_NET_IF_DL_H
#   define YETI_CONFIG_HAVE_SOCKADDR_DL
#   if !defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE
#       define YETI_CONFIG_HAVE_NET_IF_TYPES_H
#       if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6)
#           define YETI_CONFIG_HAVE_STAT_ST_BIRTHTIME
#       endif
#   endif
#endif

/*----------------------------------------------------------------------
|   默认类型/函数定义
+---------------------------------------------------------------------*/
#if !defined(YETI_FORMAT_64)
#   define YETI_FORMAT_64 "ll"
#endif

#if !defined(YETI_POINTER_TO_LONG)
#   define YETI_POINTER_TO_LONG(_p) ((long)(_p))
#endif

#if !defined(YETI_CONFIG_INT64_TYPE)
#   define YETI_CONFIG_INT64_TYPE long long
#endif

#if !defined(YETI_INT64_C)
#   define YETI_INT64_C(_x) _x##LL
#endif

#if !defined(YETI_UINT64_C)
#   define YETI_UINT64_C(_x) _x##ULL
#endif

#if !defined(YETI_snprintf)
#   define YETI_snprintf snprintf
#endif

#if !defined(YETI_strcpy)
#   define YETI_strcpy strcpy
#endif

#if !defined(YETI_strncpy)
#   define YETI_strncpy strncpy
#endif

#if !defined(YETI_vsnprintf)
#   define YETI_vsnprintf vsnprintf
#endif

#if !defined(YETI_LocalFunctionName)
#   define YETI_LocalFunctionName (NULL)
#endif

#if !defined(YETI_CONFIG_THREAD_STACK_SIZE)
#   define YETI_CONFIG_THREAD_STACK_SIZE 0
#endif

#if !defined(YETI_fseek)
#   define YETI_fseek fseeko
#endif

#if !defined(YETI_ftell)
#   define YETI_ftell ftello
#endif

#if !defined(YETI_stat)
#   define YETI_stat stat
#endif

#if !defined(YETI_stat_struct)
#   define YETI_stat_struct struct stat
#endif

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if defined(DMALLOC)
#   include <dmalloc.h>
#endif

#endif // _CXL_YETI_CONFIG_H_
