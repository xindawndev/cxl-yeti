#ifndef _CXL_YETI_TYPES_H_
#define _CXL_YETI_TYPES_H_

#include "YetiConfig.h"

/*----------------------------------------------------------------------
|   sized types (this assumes that ints are 32 bits)
+---------------------------------------------------------------------*/
typedef YETI_CONFIG_INT64_TYPE          YETI_Int64;
typedef unsigned YETI_CONFIG_INT64_TYPE YETI_UInt64;
typedef unsigned int                    YETI_UInt32;
typedef int                             YETI_Int32;
typedef unsigned short                  YETI_UInt16;
typedef short                           YETI_Int16;
typedef unsigned char                   YETI_UInt8;
typedef char                            YETI_Int8;
typedef float                           YETI_Float;

/*----------------------------------------------------------------------
|   named types
+---------------------------------------------------------------------*/
typedef int            YETI_Result;
typedef unsigned int   YETI_Cardinal;   // 基数：一、二、三……
typedef unsigned int   YETI_Ordinal;    // 序数：第一、第二、第三……
typedef YETI_UInt32    YETI_Size;
typedef YETI_UInt64    YETI_LargeSize;
typedef YETI_Int32     YETI_Offset;
typedef YETI_UInt64    YETI_Position;
typedef YETI_Int32     YETI_Timeout;
typedef void           YETI_Interface;
typedef YETI_UInt8     YETI_Byte;
typedef YETI_UInt32    YETI_Flags;
typedef void*          YETI_Any;
typedef const void*    YETI_AnyConst;

/*----------------------------------------------------------------------
|   limits
+---------------------------------------------------------------------*/
#if defined(YETI_CONFIG_HAVE_LIMITS_H)
#   include <limits.h>
#endif

#if !defined(YETI_INT_MIN)
#   if defined(YETI_CONFIG_HAVE_INT_MIN)
#       define YETI_INT_MIN INT_MIN
#   endif
#endif

#if !defined(YETI_INT_MAX)
#   if defined(YETI_CONFIG_HAVE_INT_MAX)
#       define YETI_INT_MAX INT_MAX
#   endif
#endif

#if !defined(YETI_UINT_MAX)
#   if defined(YETI_CONFIG_HAVE_UINT_MAX)
#       define YETI_UINT_MAX UINT_MAX
#   endif
#endif

#if !defined(YETI_LONG_MIN)
#   if defined(YETI_CONFIG_HAVE_LONG_MIN)
#       define YETI_LONG_MIN LONG_MIN
#   endif
#endif

#if !defined(YETI_LONG_MAX)
#   if defined(YETI_CONFIG_HAVE_LONG_MAX)
#       define YETI_LONG_MAX LONG_MAX
#   endif
#endif

#if !defined(YETI_ULONG_MAX)
#   if defined(YETI_CONFIG_HAVE_ULONG_MAX)
#       define YETI_ULONG_MAX ULONG_MAX
#   endif
#endif

#if !defined(YETI_INT32_MAX)
#   define YETI_INT32_MAX 0x7FFFFFFF
#endif

#if !defined(YETI_INT32_MIN)
#   define YETI_INT32_MIN (-YETI_INT32_MAX - 1) 
#endif

#if !defined(YETI_UINT32_MAX)
    #define YETI_UINT32_MAX 0xFFFFFFFF
#endif

#if !defined(YETI_INT64_MAX)
#   if defined(YETI_CONFIG_HAVE_LLONG_MAX)
#d      efine YETI_INT64_MAX LLONG_MAX
#   else
#       define YETI_INT64_MAX 0x7FFFFFFFFFFFFFFFLL
#   endif
#endif

#if !defined(YETI_INT64_MIN)
#   if defined(YETI_CONFIG_HAVE_LLONG_MIN)
#       define YETI_INT64_MIN LLONG_MIN
#   else
#       define YETI_INT64_MIN (-YETI_INT64_MAX - 1LL) 
#   endif
#endif

#if !defined(YETI_UINT64_MAX)
#   if defined(YETI_CONFIG_HAVE_ULLONG_MAX)
#       define YETI_UINT64_MAX ULLONG_MAX
#   else
#       define YETI_UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#   endif
#endif

#endif // _CXL_YETI_TYPES_H_
