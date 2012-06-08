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
            _copy_string(copy, m_chars_);
            delete _get_buffer();
        } else {
            copy[0] = '\0';
        }
        m_chars_ = copy;
    }
}

void String::assign(const char * chars, YETI_Size size)
{
    if (chars == NULL || size == 0) {
        _reset();
    } else {
        for (unsigned int i = 0; i < size - 1; ++i) {
            if (chars[i] == '\0') {
                if (i == 0) {
                    _reset();
                    return;
                } else {
                    size = i;
                    break;
                }
            }
        }

        _prepare_to_write(size);
        _copy_buffer(m_chars_, chars, size);
        m_chars_[size] = '\0';
    }
}

String & String::operator =(const char * str)
{
    if (str == NULL) {
        _reset();
    } else {
        YETI_Size length = _string_length(str);
        if (length == 0) {
            _reset();
        } else {
            _copy_string(_prepare_to_write(length), str);
        }
    }

    return *this;
}

String & String::operator =(const String & str)
{
    if (this != &str) {
        assign(str.get_chars(), str.get_length());
    }

    return *this;
}

YETI_UInt32 String::get_hash32() const
{
    return fnv1a_hashstr32(get_chars());
}

YETI_UInt64 String::get_hash64() const
{
    return fnv1a_hashstr64(get_chars());
}

void String::append(const char * chars, YETI_Size size)
{
    if (chars == NULL || size == 0) return;
    YETI_Size old_size = get_length();
    YETI_Size new_size = old_size + size;

    reserve(new_size);

    _copy_buffer(m_chars_ + old_size, chars, size);
    m_chars_[new_size] = '\0';

    _get_buffer()->set_length(new_size);
}

int String::compare(const char * s, bool ignore_case /* = false */) const
{
    return String::compare(get_chars(), s, ignore_case);
}

int String::compare(const char * s1, const char * s2, bool ignore_case /* = false */)
{
    const char * r1 = s1;
    const char * r2 = s2;

    if (ignore_case) {
        while (_uppercase(*r1) == _uppercase(*r2)) {
            if (*r1++ == '\0') {
                return 0;
            }
            r2++;
        }
        return _uppercase(*r1) - _uppercase(*r2);
    } else {
        while (*r1 == *r2) {
            if (*r1++ == '\0') {
                return 0;
            }
            r2++;
        }
        return (*r1 - *r2);
    }
}

int String::compare_n(const char * s, YETI_Size count, bool ignore_case /* = false */) const
{
    return String::compare_n(get_chars(), s, count, ignore_case);
}

int String::compare_n(const char * s1, const char * s2, YETI_Size count, bool ignore_case /* = false */)
{
    const char * me = s1;

    if (ignore_case) {
        for (unsigned int i = 0; i < count; ++i) {
            if (_uppercase(me[i]) != _uppercase(s2[i])) {
                return (int)(_uppercase(me[i]) - _uppercase(s2[i]));
            }
        }
        return 0;
    } else {
        for (unsigned int i = 0; i < count; ++i) {
            if (me[i] != s2[i]) {
                return (int)(me[i] - s2[i]);
            }
        }

        return 0;
    }
}

List<String> String::split(const char * separator) const
{
    List<String> result;
    YETI_Size    separator_length = StringLength(separator);
    if (separator_length == 0) {
        result.add(*this);
        return result;
    }

    int current = 0;
    int next;
    do {
        next = find(separator, current);
        unsigned int end = (next > 0 ? (unsigned int)next: get_length());
        result.add(sub_string(current, end -current));
        current = next + separator_length;
    } while(next >= 0);

    return result;
}

Array<String> String::split_any(const char * separator) const
{
    Array<String> result((get_length() >> 1) + 1);
    if (StringLength(separator) == 0) {
        result.add(*this);
        return result;
    }

    int current = 0;
    int next;
    do {
        next = find_any(separator, current);
        unsigned int end = (next >= 0 ? (unsigned int)next : get_length());
        result.add(sub_string(current, end-current));
        current = next + 1;
    } while (next >= 0);

    return result;
}

NAMEEND
