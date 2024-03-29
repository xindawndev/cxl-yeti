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

YETI_Int32 String::npos = -1;
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
            // ʹ�ÿո����
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
        while (uppercase(*r1) == uppercase(*r2)) {
            if (*r1++ == '\0') {
                return 0;
            }
            r2++;
        }
        return uppercase(*r1) - uppercase(*r2);
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
            if (uppercase(me[i]) != uppercase(s2[i])) {
                return (int)(uppercase(me[i]) - uppercase(s2[i]));
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
        while (uppercase(*str) == uppercase(*sub)) {
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
            if (uppercase(*src) == uppercase(c)) {
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
                if (uppercase(*src) == uppercase(s[i])) {
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
            if (uppercase(src[i]) == uppercase(c)) {
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
        *dst = lowercase(*dst);
        dst++;
    }
}

void String::make_uppercase()
{
    const char * src = get_chars();
    char *dst = const_cast<char *>(src);
    while (*dst != '\0') {
        *dst = uppercase(*dst);
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
    result.make_uppercase();
    return result;
}

const String & String::replace(char a, char b)
{
    if (m_chars_ == NULL || a == '\0' || b == '\0') return *this;
    char * src = m_chars_;
    while (*src) {
        if (*src == a) *src = b;
        src++;
    }
    return *this;
}

const String & String::replace(char a, const char * s)
{
    if (m_chars_ ==  NULL || a == '\0' || s == NULL || s[0] == '\0') return *this;
    if (StringLength(s) == 1) return replace(a, s[0]);

    String dst;
    char * src = m_chars_;

    dst.reserve(get_length());
    while (*src) {
        if (*src == a) {
            dst += src;
        } else {
            dst += *src;
        }
        src++;
    }

    assign(dst.get_chars(), dst.get_length());
    return *this;
}

const String & String::replace(const char * before, const char * after)
{
    YETI_Size size_before = StringLength(before);
    YETI_Size size_after = StringLength(after);
    int index = find(before);
    while (index != YETI_STRING_SEARCH_FAILED) {
        erase(index, size_before);
        insert(after, index);
        index = find(before, index + size_after);
    }

    return *this;
}

const String & String::insert(const char * s, YETI_Ordinal where /* = 0 */)
{
    if (s == NULL || where > get_length()) return *this;

    YETI_Size str_length = _string_length(s);
    if (str_length == 0) return *this;

    YETI_Size old_length = get_length();
    YETI_Size new_length = str_length + get_length();

    char * src = m_chars_;
    char * nst = Buffer::create(new_length, new_length);
    char * dst = nst;

    if (where > 0) {
        _copy_buffer(dst, src, where);
        src += where;
        dst += where;
    }

    _copy_string(dst, src);
    dst += str_length;
    if (old_length > where) {
        _copy_string(dst, src);
    }

    if (m_chars_) delete _get_buffer();
    m_chars_ = nst;
    return *this;
}

const String & String::erase(YETI_Ordinal start, YETI_Cardinal count /* = 1 */)
{
    YETI_Size length = get_length();
    if (start + count > length) {
        if (start > length) return *this;
        count = length - start;
    }
    if (count == 0) return *this;
    _copy_string(m_chars_ + start, m_chars_ + start + count);
    _get_buffer()->set_length(length - count);

    return *this;
}

YETI_Result String::to_integer(int & value, bool relaxed /* = true */) const
{
    return parse_integer(get_chars(), value, relaxed);
}

YETI_Result String::to_integer(unsigned int & value, bool relaxed /* = true */) const
{
    return parse_integer(get_chars(), value, relaxed);
}

YETI_Result String::to_integer(long & value, bool relaxed /* = true */) const
{
    return parse_integer(get_chars(), value, relaxed);
}

YETI_Result String::to_integer(unsigned long & value, bool relaxed /* = true */) const
{
    return parse_integer(get_chars(), value, relaxed);
}

YETI_Result String::to_integer32(YETI_Int32 & value, bool relaxed /* = true */) const
{
    return parse_integer32(get_chars(), value, relaxed);
}

YETI_Result String::to_integer32(YETI_UInt32 & value, bool relaxed /* = true */) const
{
    return parse_integer32(get_chars(), value, relaxed);
}

YETI_Result String::to_integer64(YETI_Int64 & value, bool relaxed /* = true */) const
{
    return parse_integer64(get_chars(), value, relaxed);
}

YETI_Result String::to_integer64(YETI_UInt64 & value, bool relaxed /* = true */) const
{
    return parse_integer64(get_chars(), value, relaxed);
}

YETI_Result String::to_float(float & value, bool relaxed /* = true */) const
{
    return parse_float(get_chars(), value, relaxed);
}

const String & String::trim_left()
{
    return trim_left(YETI_STRINGS_WHITESPACE_CHARS);
}

const String & String::trim_left(char c)
{
    char s[2] = {c, 0};
    return trim_left((const char *)s);
}

const String & String::trim_left(const char * chars)
{
    if (m_chars_ == NULL) return *this;
    const char * s = m_chars_;
    while (char c = *s) {
        const char * x = chars;
        while (*x) {
            if (*x == c) break;
            x++;
        }
        if (*x == 0) break; // not found
        s++;
    }
    if (s == m_chars_) {
        // nothing was trimmed
        return *this;
    }

    char * d = m_chars_;
    _get_buffer()->set_length(get_length() - (s - d));
    while ((*d++ = *s++)) {};
    return *this;
}

const String & String::trim_right()
{
    return trim_right(YETI_STRINGS_WHITESPACE_CHARS);
}

const String & String::trim_right(char c)
{
    char s[2] = {c, 0};
    return trim_right((const char *)s);
}

const String & String::trim_right(const char * chars)
{
    if (m_chars_ == NULL || m_chars_[0] == '\0') return *this;
    char * tail = m_chars_ + get_length() - 1;
    char * s = tail;
    while (s != m_chars_ - 1) {
        const char * x = chars;
        while (*x) {
            if (*x == *s) {
                *s = '\0';
                break;
            }
            x++;
        }
        if (*x == 0) break; // not found
        s--;
    }
    if (s == tail) {
        // nothing was trimmed
        return *this;
    }
    _get_buffer()->set_length(1 + (int)(s - m_chars_));
    return *this;
}

const String & String::trim()
{
    trim_left();
    return trim_right();
}

const String & String::trim(char c)
{
    char s[2] = {c, 0};
    trim_left((const char*)s);
    return trim_right((const char*)s);
}

const String & String::trim(const char * chars)
{
    trim_left(chars);
    return trim_right(chars);
}

String operator +(const String & s1, const char * s2)
{
    // shortcut
    if (s2 == NULL) return String(s1);

    // measure strings
    YETI_Size s1_length = s1.get_length();
    YETI_Size s2_length = String::_string_length(s2);

    // allocate space for the new string
    String result;
    char* start = result._prepare_to_write(s1_length + s2_length);

    // concatenate the two strings into the result
    String::_copy_buffer(start, s1, s1_length);
    String::_copy_string(start + s1_length, s2);

    return result;
}

String operator+(const char * s1, const String & s2)
{
    // shortcut
    if (s1 == NULL) return String(s2);

    // measure strings
    YETI_Size s1_length = String::_string_length(s1);
    YETI_Size s2_length = s2.get_length();

    // allocate space for the new string
    String result;
    char* start = result._prepare_to_write(s1_length + s2_length);

    // concatenate the two strings into the result
    String::_copy_buffer(start, s1, s1_length);
    String::_copy_string(start+s1_length, s2.get_chars());

    return result;
}

String operator+(const String & s1, char c)
{
    // allocate space for the new string
    String result;
    result.reserve(s1.get_length() + 1);

    // append
    result = s1;
    result += c;

    return result;
}

NAMEEND
