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

String String::join(List<String> & args, const char * separator)
{
    String output;
    List<String>::iterator arg = args.get_first_item();
    while (arg) {
        output += *arg;
        if (++arg) output += separator;
    }

    return output;
}

String String::sub_string(YETI_Ordinal first, YETI_Size length) const
{
    if (first >= get_length()) {
        first = get_length();
        length = 0;
    } else if (first + length >= get_length()) {
        length = get_length() - first;
    }

    return String(get_chars() + first, length);
}

/*----------------------------------------------------------------------
|   returns:
|       1 if str starts with sub,
|       0 if str is large enough but does not start with sub
|       -1 if str is too short to start with sub
+---------------------------------------------------------------------*/
static inline int _string_starts_with(const char * str, const char * sub, bool ignore_case)
{
    if (ignore_case) {
        while (_uppercase(*str) == _uppercase(*sub)) {
            if (*str++ == '\0') {
                return 1;
            }
            sub++;
        }
    } else {
        while (*str == *sub) {
            if (*str++ == '\0') {
                return 1;
            }
            sub++;
        }
    }

    return (*sub == '\0') ? 1 : (*str == '\0' ? -1 : 0);
}

bool String::starts_with(const char * s, bool ignore_case /* = false */) const
{
    if (s == NULL) return false;
    return _string_starts_with(get_chars(), s, ignore_case) == 1;
}

bool String::ends_with(const char * s, bool ignore_case /* = false */) const
{
    if (s == NULL) return false;
    YETI_Size str_length = StringLength(s);
    if (str_length > get_length()) return false;
    return _string_starts_with(get_chars() + get_length() - str_length, s, ignore_case) == 1;
}

int String::find(const char * s, YETI_Ordinal start /* = 0 */, bool ignore_case /* = false */) const
{
    if (s == NULL || start >= get_length()) return -1;

    const char * src = m_chars_ + start;
    while (*src) {
        int cmp = _string_starts_with(src, s, ignore_case);
        switch (cmp) {
            case -1:
                return -1;
            case 1:
                return (int)(src - m_chars_);
        }

        src++;
    }

    return -1;
}

int String::find(char c, YETI_Ordinal start /* = 0 */, bool ignore_case /* = false */) const
{
    if (start >= get_length()) return -1;
    const char * src = m_chars_ + start;
    if (ignore_case) {
        while (*src) {
            if (_uppercase(*src) == _uppercase(c)) {
                return (int)(src - m_chars_);
            }
            src++;
        }
    } else {
        while (*src) {
            if (*src == c) {
                return (int)(src - m_chars_);
            }
            src++;
        }
    }

    return -1;
}

int String::find_any(const char * s, YETI_Ordinal start, bool ignore_case /* = false */) const
{
    if (start >= get_length()) return -1;
    const char * src = m_chars_ + start;
    if (ignore_case) {
        while (*src) {
            for (YETI_Size i = 0; i < StringLength(s); ++i) {
                if (_uppercase(*src) == _uppercase(s[i])) {
                    return (int)(src - m_chars_);
                }
            }
            src++;
        }
    } else {
        while (*src) {
            for (YETI_Size i = 0; i < StringLength(s); ++i) {
                if (*src == s[i]) {
                    return (int)(src - m_chars_);
                }
            }
            src++;
        }
    }
    return -1;
}

int String::reverse_find(const char * s, YETI_Ordinal start /* = 0 */, bool ignore_case /* = false */) const
{
    if (s == NULL || *s == '\0') return -1;
    YETI_Size my_length = get_length();
    YETI_Size str_length = StringLength(s);
    int i = my_length - start - str_length;
    const char * src = get_chars();
    if (i < 0) return -1;
    for (; i > 0; --i) {
        int cmp = _string_starts_with(src + i, s, ignore_case);
        if (cmp == 1) {
            return i;
        }
    }

    return -1;
}

int String::reverse_find(char c, YETI_Ordinal start /* = 0 */, bool ignore_case /* = false */) const
{
    YETI_Size length = get_length();
    int i = length - start - 1;
    if (i < 0) return -1;

    const char * src = get_chars();
    if (ignore_case) {
        for (; i >= 0; --i) {
            if (_uppercase(src[i]) == _uppercase(c)) {
                return i;
            }
        }
    } else {
        for (; i >= 0; --i) {
            if (src[i] == c) return i;
        }
    }

    return -1;
}

void String::make_lowercase()
{
    const char * src = get_chars();
    char * dst = const_cast<char *>(src);
    while (*dst != '\0') {
        *dst = _lowercase(*dst);
        dst++;
    }
}

void String::make_uppercase()
{
    const char * src = get_chars();
    char *dst = const_cast<char *>(src);
    while (*dst != '\0') {
        *dst = _uppercase(*dst);
        dst++;
    }
}

String String::to_lowercase() const
{
    String result(*this);
    result.make_lowercase();
    return result;
}

String String::to_uppercase() const
{
    String result(*this);
    result.to_uppercase();
    return result;
}

NAMEEND
