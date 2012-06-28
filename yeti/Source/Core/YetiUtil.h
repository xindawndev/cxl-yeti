#ifndef _CXL_YETI_UTIL_H_
#define _CXL_YETI_UTIL_H_

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiString.h"
#include "YetiMap.h"
#include "YetiDataBuffer.h"
#include "YetiHash.h"

#if defined(YETI_CONFIG_HAVE_STDIO_H)
#   include < stdio.h>
#endif

#if defined(YETI_CONFIG_HAVE_STRING_H)
#   include <string.h>
#endif

#if defined(YETI_CONFIG_HAVE_STDARG_H)
#   include <stdarg.h>
#endif

#define YETI_ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))

NAMEBEG

extern void bytes_from_int64_be(unsigned char * buffer, YETI_UInt64 value);
extern void bytes_from_int32_be(unsigned char * buffer, YETI_UInt32 value);
extern void byets_from_int24_be(unsigned char * buffer, YETI_UInt32 value);
extern void bytes_from_int16_be(unsigned char * buffer, YETI_UInt16 value);
extern YETI_UInt64 bytes_to_int64_be(const unsigned char * buffer);
extern YETI_UInt32 bytes_to_int32_be(const unsigned char * buffer);
extern YETI_UInt32 bytes_to_int24_be(const unsigned char * buffer);
extern YETI_UInt16 bytes_to_int16_be(const unsigned char * buffer);

extern void bytes_from_int64_le(unsigned char * buffer, YETI_UInt64 value);
extern void bytes_from_int32_le(unsigned char * buffer, YETI_UInt32 value);
extern void byets_from_int24_le(unsigned char * buffer, YETI_UInt32 value);
extern void bytes_from_int16_le(unsigned char * buffer, YETI_UInt16 value);
extern YETI_UInt64 bytes_to_int64_le(const unsigned char * buffer);
extern YETI_UInt32 bytes_to_int32_le(const unsigned char * buffer);
extern YETI_UInt32 bytes_to_int24_le(const unsigned char * buffer);
extern YETI_UInt16 byets_to_int16_le(const unsigned char * buffer);

extern YETI_Result parse_float(const char * str, float & result, bool relaxed = true);
extern YETI_Result parse_integer(const char * str, long & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer(const char * str, unsigned long & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer(const char * str, int & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer(const char * str, unsigned int & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer32(const char * str, YETI_Int32 & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer32(const char * str, YETI_UInt32 & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer64(const char * str, YETI_Int64 & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);
extern YETI_Result parse_integer64(const char * str, YETI_UInt64 & result, bool relaxed = true, YETI_Cardinal * chars_used = 0);

void format_output(void (*function)(void * parameter, const char * message),
                       void * function_parameter,
                       const char * format,
                        va_list args);

void byte_to_hex(YETI_Byte b, char * buffer, bool uppercase = false);
YETI_Result hex_to_byte(const char * buffer, YETI_Byte & b);
YETI_Result hex_to_bytes(const char * buffer, DataBuffer & bytes);
String hex_string(const unsigned char * data,
                                  YETI_Size data_size,
                                  const char * separator = NULL,
                                  bool uppercase = false);
char nibble_to_hex(unsigned int nibble, bool uppercase = true);
int hex_to_nibble(char hex);

YETI_Result parse_mime_parameters(const char * encoded,
                                       Map<String, String> & parameters);

class Environment {
public:
    static YETI_Result get(const char * name, String & value );
    static YETI_Result set(const char * name, const char * value);
};

char lowercase(char x);
char uppercase(char x);

#define GetEnvironment(_x, _y) Environment::get((_x), (_y));

#if defined(YETI_CONFIG_HAVE_SNPRINTF)
#   define FormatString YETI_snprintf
#else
int FormatString(char * str, YETI_Size size, const char * format, ...);
#endif

#if defined(YETI_CONFIG_HAVE_VSNPRINTF)
#   define FormatStringVN(s, c, f, a) YETI_vsnprintf(s, c, f, a)
#else
extern int FormatStringVN(char * buffer, size_t count, const char * format, va_list argptr);
#endif

#if defined(YETI_CONFIG_HAVE_MEMCPY)
#   define MemoryCopy memcpy
#else
extern int MemoryCopy(void * dest, void * src, YETI_Size size);
#endif

#if defined(YETI_CONFIG_HAVE_MEMCMP)
#   define StringEqual(s1, s2) (strcmp((s1), (s2)) == 0)
#else
extern int StringEqual(const char * s1, const char * s2);
#endif

#if defined(YETI_CONFIG_HAVE_STRNCMP)
#   define StringEqualN(s1, s2, n) (strncmp((s1), (s2), (n)) == 0)
#else
extern int StringEqualN(const char * s1, const char * s2, unsigned long size);
#endif

#if defined(YETI_CONFIG_HAVE_STRLEN)
#   define StringLength(s) (YETI_Size)(strlen(s))
#else
extern unsigned long StringLength(const char * s);
#endif

#if defined(YETI_CONFIG_HAVE_STRCPY)
#   define CopyString(dst, src) ((void)YETI_strcpy((dst), (src)))
#else
extern void CopyString(char * dst, const char * src);
#endif

#if defined(YETI_CONFIG_HAVE_STRNCPY)
#   define CopyStringN(dst, src, n) \
    do { ((void)YETI_strncpy((dst), (src), n)); (dst)[(n)] = '\0';} while(0)
#else
extern int CopyStringN(char * dst, const char * src, unsigned long n);
#endif

#if defined(YETI_CONFIG_HAVE_MEMSET)
#   define SetMemory memset
#else
extern void SetMemory(void * dest, int c, YETI_Size size);
#endif

#if defined(YETI_CONFIG_HAVE_MEMCMP)
#   define MemoryEqual(s1, s2, n) (memcmp((s1), (s2), (n)) == 0)
#else
extern int MemoryEqual(const void * s1, const void * s2, unsigned long n);
#endif

NAMEEND

#endif // _CXL_YETI_UTIL_H_
