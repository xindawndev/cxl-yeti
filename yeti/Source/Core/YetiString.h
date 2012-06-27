#ifndef _CXL_YETI_STRING_H_
#define _CXL_YETI_STRING_H_

#include "YetiConfig.h"
#if defined(YETI_CONFIG_HAVE_NEW_H)
#   include <new>
#endif
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiList.h"
#include "YetiArray.h"
#include "YetiDebug.h"
#include "YetiHash.h"

NAMEBEG

const int YETI_STRING_SEARCH_FAILED = -1;

class String
{
public:
    static YETI_Int32 npos;
public:
    static String from_integer(YETI_Int64 value);
    static String from_u_integer(YETI_UInt64 value);
    static String format(const char * format, ...);

    String(const String & str);
    String(const char * str);
    String(const char * str, YETI_Size length);
    String(char c, YETI_Cardinal repeat = 1);
    String() : m_chars_(NULL) {}
    ~String() { if (m_chars_) _get_buffer()->destory();}

    bool        is_empty() const { return m_chars_ == NULL || _get_buffer()->get_length() == 0; }
    YETI_Size   get_length() const { return m_chars_ ? _get_buffer()->get_length() : 0; }
    YETI_Size   get_capacity() const { return m_chars_ ? _get_buffer()->get_allocated() : 0; }
    YETI_Result set_length(YETI_Size length, bool pad = false);
    void        assign(const char * chars, YETI_Size size);
    void        append(const char * chars, YETI_Size size);
    void        append(const char * chars) { append(chars, _string_length(chars)); }
    int         compare(const char * s, bool ignore_case = false) const;
    static int  compare(const char * s1, const char * s2, bool ignore_case = false);
    int         compare_n(const char * s, YETI_Size count, bool ignore_case = false) const;
    static int  compare_n(const char * s1, const char * s2, YETI_Size count, bool ignore_case = false);

    String      sub_string(YETI_Ordinal first, YETI_Size length) const;
    String      sub_string(YETI_Ordinal first) const {
        return sub_string(first, get_length());
    }
    String      left(YETI_Size length) const {
        return sub_string(0, length);
    }
    String      right(YETI_Size length) const {
        return length >= get_length() ? *this : sub_string(get_length() - length, get_length());
    }

    List<String>    split(const char * separator) const;
    Array<String>   split_any(const char * separator) const;
    static String   join(List<String> & args, const char * separator);

    void            reserve(YETI_Size length);

    YETI_UInt32     get_hash32() const;
    YETI_UInt64     get_hash64() const;

    String          to_lowercase() const;
    String          to_uppercase() const;
    YETI_Result     to_integer(int & value, bool relaxed = true) const;
    YETI_Result     to_integer(unsigned int & value, bool relaxed = true) const;
    YETI_Result     to_integer(long & value, bool relaxed = true) const;
    YETI_Result     to_integer(unsigned long & value, bool relaxed = true) const;
    YETI_Result     to_integer32(YETI_Int32 & value, bool relaxed = true) const;
    YETI_Result     to_integer32(YETI_UInt32 & value, bool relaxed = true) const;
    YETI_Result     to_integer64(YETI_Int64 & value, bool relaxed = true) const;
    YETI_Result     to_integer64(YETI_UInt64 & value, bool relaxed = true) const;
    YETI_Result     to_float(float & value, bool relaxed = true) const;

    void            make_lowercase();
    void            make_uppercase();
    const String &  replace(char a, char b);
    const String &  replace(char a, const char * s);

    int             find(char c, YETI_Ordinal start = 0, bool ignore_case = false) const;
    int             find(const char * s, YETI_Ordinal start = 0, bool ignore_case = false) const;
    int             find_any(const char * s, YETI_Ordinal start, bool ignore_case = false) const;
    int             reverse_find(char c, YETI_Ordinal start = 0, bool ignore_case = false) const;
    int             reverse_find(const char * s, YETI_Ordinal start = 0, bool ignore_case = false) const;
    bool            starts_with(const char * s, bool ignore_case = false) const;
    bool            ends_with(const char * s, bool ignore_case = false) const;

    const String &  insert(const char * s, YETI_Ordinal where = 0);
    const String &  erase(YETI_Ordinal start, YETI_Cardinal count = 1);
    const String &  replace(const char * before, const char * after);
    //void replace(YETI_Ordinal start, YETI_Cardinal count, const char * s);
    const String &  trim_left();
    const String &  trim_left(char c);
    const String &  trim_left(const char * chars);
    const String &  trim_right();
    const String &  trim_right(char c);
    const String &  trim_right(const char * chars);
    const String &  trim();
    const String &  trim(char c);
    const String &  trim(const char * chars);

    operator        char*() const           { return m_chars_ ? m_chars_ : &empty_string_; }
    operator const  char *() const          { return m_chars_ ? m_chars_ : &empty_string_; }
    const char *    get_chars() const       { return m_chars_ ? m_chars_ : &empty_string_; }
    char *          use_chars()             { return m_chars_ ? m_chars_ : &empty_string_; }

    String &        operator =(const char * str);
    String &        operator =(const String & str);
    String &        operator =(char c);
    const String &  operator +=(const String & s) {
        append(s.get_chars(), s.get_length());
        return *this;
    }
    const String &  operator +=(const char * s) {
        append(s);
        return *this;
    }
    const String &  operator +=(char c) {
        append(&c, 1);
        return *this;
    }
    char            operator [](int index) const {
        YETI_ASSERT((unsigned int)index < get_length());
        return get_chars()[index];
    }
    char &          operator [](int index) {
        YETI_ASSERT((unsigned int)index < get_length());
        return use_chars()[index];
    }

    friend String   operator +(const String & s1, const String & s2) {
        return s1 + s2.get_chars();
    }
    friend String   operator +(const String & s1, const char * s2);
    friend String   operator +(const char * s1, const String & s2);
    friend String   operator +(const String & s, char c);
    friend String   operator +(char c, const String & s);

protected:
    class Buffer {
    public:
        static Buffer * allocate(YETI_Size allocated, YETI_Size length) {
            void * mem = ::operator new(sizeof(Buffer) + allocated + 1);
            return new (mem) Buffer(allocated, length);
        }

        static char * create(YETI_Size allocated, YETI_Size length = 0) {
            Buffer * shared = allocate(allocated, length);
            return shared->get_chars();
        }

        static char * create(const char * copy) {
            YETI_Size length = _string_length(copy);
            Buffer * shared = allocate(length, length);
            _copy_string(shared->get_chars(), copy);
            return shared->get_chars();
        }

        static char * create(const char * copy, YETI_Size length) {
            Buffer * shared = allocate(length, length);
            _copy_buffer(shared->get_chars(), copy, length);
            shared->get_chars()[length] = '\0';
            return shared->get_chars();
        }

        static char * create(char c, YETI_Cardinal repeat) {
            Buffer * shared = allocate(repeat, repeat);
            char * s = shared->get_chars();
            while (repeat--) {
                *s++ = c;
            }
            *s = '\0';
            return shared->get_chars();
        }

        char * get_chars() {
            return reinterpret_cast<char *>(this + 1);
        }

        YETI_Size get_length() const { return m_length_; }
        void set_length(YETI_Size length) { m_length_ = length; }
        YETI_Size get_allocated() const { return m_allocated_; }
        void destory() { ::operator delete((void *)this); }

    private:
        Buffer(YETI_Size allocated, YETI_Size length = 0)
            : m_length_(length)
            , m_allocated_(allocated) {}

        YETI_Cardinal m_length_;
        YETI_Cardinal m_allocated_;
    };
    char * m_chars_;

private:
    friend class Buffer;
    static char empty_string_;

    Buffer * _get_buffer() const {
        return reinterpret_cast<Buffer *>(m_chars_) - 1;
    }

    void _reset() {
        if (m_chars_ != NULL) {
            delete m_chars_;
            m_chars_ = NULL;
        }
    }

    char * _prepare_to_write(YETI_Size length);
    void _prepare_to_append(YETI_Size length, YETI_Size allocate);

    static void _copy_string(char * dst, const char * src) {
        while ((*dst++ = *src++)){}
    }

    static void _copy_buffer(char * dst, const char * src, YETI_Size size) {
        while (size--) *dst++ = *src++;
    }

    static YETI_Size _string_length(const char * str) {
        YETI_Size length = 0;
        while (*str++) length++;
        return length;
    }

};

inline bool operator==(const String& s1, const String& s2) { 
    return s1.compare(s2) == 0; 
}
inline bool operator==(const String& s1, const char* s2) {
    return s1.compare(s2) == 0; 
}
inline bool operator==(const char* s1, const String& s2) {
    return s2.compare(s1) == 0; 
}
inline bool operator!=(const String& s1, const String& s2) {
    return s1.compare(s2) != 0; 
}
inline bool operator!=(const String& s1, const char* s2) {
    return s1.compare(s2) != 0; 
}
inline bool operator!=(const char* s1, const String& s2) {
    return s2.compare(s1) != 0; 
}
inline bool operator<(const String& s1, const String& s2) {
    return s1.compare(s2) < 0; 
}
inline bool operator<(const String& s1, const char* s2) {
    return s1.compare(s2) < 0; 
}
inline bool operator<(const char* s1, const String& s2) {
    return s2.compare(s1) > 0; 
}
inline bool operator>(const String& s1, const String& s2) {
    return s1.compare(s2) > 0; 
}
inline bool operator>(const String& s1, const char* s2) {
    return s1.compare(s2) > 0; 
}
inline bool operator>(const char* s1, const String& s2) {
    return s2.compare(s1) < 0; 
}
inline bool operator<=(const String& s1, const String& s2) {
    return s1.compare(s2) <= 0; 
}
inline bool operator<=(const String& s1, const char* s2) {
    return s1.compare(s2) <= 0; 
}
inline bool operator<=(const char* s1, const String& s2) {
    return s2.compare(s1) >= 0; 
}
inline bool operator>=(const String& s1, const String& s2) {
    return s1.compare(s2) >= 0; 
}
inline bool operator>=(const String& s1, const char* s2) {
    return s1.compare(s2) >= 0; 
}
inline bool operator>=(const char* s1, const String& s2) {
    return s2.compare(s1) <= 0; 
}

template <>
struct Hash<String>
{
    YETI_UInt32 operator()(const String & s) { return s.get_hash32(); }
};

NAMEEND

#endif // _CXL_YETI_STRING_H_
