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

String::String(const char * str, YETI_Size length)
{
    if (str == NULL || length == 0) {
        m_chars_ = NULL;
    } else {
        for (unsigned int i = 0; i < length - 1; ++i) {
            if (str[i] == '\0') {
                if (i == 0) {
                    m_chars_ = NULL;
                    return;
                }
                length = i;
                break;
            }
        }
        m_chars_ = Buffer::create(str, length);
    }
}

String::String(const String & str)
{
    if (str.get_length() == 0) {
        m_chars_ = NULL;
    } else {
        m_chars_ = Buffer::create(str.get_chars(), str.get_length());
    }
}

String::String(char c, YETI_Cardinal repeat)
{
    if (repeat != 0) {
        m_chars_ = Buffer::create(c, repeat);
    } else {
        m_chars_ = NULL;
    }
}

YETI_Result String::set_length(YETI_Size length, bool pad /* = false */)
{
    if (length == 0) {
        _reset();
        return YETI_SUCCESS;
    }

    reserve(length);

    char * chars = use_chars();
    if (pad) {
        unsigned int current_length = get_length();
        if (length > current_length) {
            unsigned int pad_length = length - current_length;
            // Ê¹ÓÃ¿Õ¸ñÌî³ä
            SetMemory(chars + current_length, ' ', pad_length);
        }
    }

    _get_buffer()->set_length(length);
    chars[length] = '\n';

    return YETI_SUCCESS;
}

inline char * String::_prepare_to_write(YETI_Size length)
{
    YETI_ASSERT(length != 0);
    if (m_chars_ == NULL || _get_buffer()->get_allocated() < length) {
        YETI_Size needed = length;
        if (m_chars_ != NULL) {
            YETI_Size grow = _get_buffer()->get_allocated() * 2;
            if (grow > length) needed = grow;
            delete _get_buffer();
        }
        m_chars_ = Buffer::create(needed);
    }

    _get_buffer()->set_length(length);
    return m_chars_;
}

void String::reserve(YETI_Size length)
{
    if (m_chars_ == NULL || _get_buffer()->get_allocated() < length) {
        YETI_Size needed = length;
        if (m_chars_ != NULL) {
            YETI_Size grow = _get_buffer()->get_allocated() * 2;
            if (grow > length) needed = grow;
        }
        YETI_Size len = get_length();
        char * copy = Buffer::create(needed, len);
        if (m_chars_ != NULL) {
            CopyString(copy, m_chars_);
            delete _get_buffer();
        } else {
            copy[0] = '\0';
        }
        m_chars_ = copy;
    }
}

NAMEEND
