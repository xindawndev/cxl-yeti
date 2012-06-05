#include "YetiString.h"

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiResults.h"
#include "YetiUtil.h"
#include "YetiDebug.h"

NAMEBEG

#define YETI_STRINGS_WHITESPACE_CHARS "\r\n\t"

const unsigned int YETI_STRING_FORMAT_BUFFER_DEFAULT_SIZE = 256;
const unsigned int YETI_STRING_FORMAT_BUFFER_MAX_SIZE     = 0x80000; // 512k

inline char _uppercase(char x) {
    return (x >= 'a' && x <= 'z') ? x&0xdf : x;
}

inline char _lowercase(char x) {
    return (x >= 'A' && x <= 'Z') ? x^32 : x;
}

char String::empty_string_ = '\0';

String String::from_integer(YETI_Int64 value)
{
    char str[32] = {0};
    char * c = &str[31];
    *c-- = '\0';
    bool negative = value < 0 ? value = -value, true : false;

    do {
        int dig = value % 10;
        *c-- = '0' + dig;
        value /= 10;
    } while (value);

    if (negative) {
        *c = '-';
    } else {
        ++c;
    }

    return String(c);
}

String String::from_u_integer(YETI_UInt64 value)
{
    char str[32] = {0};
    char * c = &str[31];
    *c = '\0';
    do {
        int dig = value % 10;
        *--c = '0' + dig;
        value /= 10;
    } while(value);

    return String(c);
}

String String::format(const char * format, ...)
{
    String result;
    YETI_Size buffer_size = YETI_STRING_FORMAT_BUFFER_DEFAULT_SIZE;

    va_list args;
    for (;;) {
        result.reserve(buffer_size);
        char * buffer = result.use_chars();
        va_start(args, format);
        int f_result = FormatStringVN(buffer, buffer_size, format, args);
        va_end(args);
        if (f_result >= (int)(buffer_size)) f_result = -1;
        if (f_result > 0) {
            result.set_length(f_result);
            break;
        }
        buffer_size *= 2;
        if (buffer_size > YETI_STRING_FORMAT_BUFFER_MAX_SIZE) break;
    }

    return result;
}

String::String(const char * str)
{
    m_chars_ = ((str == NULL) ? NULL : Buffer::create(str));
}

NAMEEND
