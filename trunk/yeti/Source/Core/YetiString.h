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
    static String from_integer(YETI_Int64 value);
    static String from_u_integer(YETI_UInt64 value);
    static String format(const char * format, ...);

    String(const String & str);
    String(const char * str);
    String(const char * str, YETI_Size length);
    String(char c, YETI_Cardinal repeat = 1);
    String() : m_chars_(NULL) {}
    ~String() { if (m_chars_) _get_buffer()->destory();}

    bool is_empty() const { return m_chars_ == NULL || _get_buffer()->get_length() == 0; }
    YETI_Size get_length() const { return m_chars_ ? _get_buffer()->get_length() : 0; }
    YETI_Size get_capacity() const { return m_chars_ ? _get_buffer()->get_allocated() : 0; }
    

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
        void destory() { ::operator delete((void *)this)}

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

NAMEEND

#endif // _CXL_YETI_STRING_H_
