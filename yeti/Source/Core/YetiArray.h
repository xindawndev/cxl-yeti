#ifndef _CXL_YETI_ARRAY_H_
#define _CXL_YETI_ARRAY_H_

#include "YetiConfig.h"
#if defined(YETI_CONFIG_HAVE_NEW_H)
#   include <new>
#endif
#include "YetiTypes.h"
#include "YetiResults.h"

NAMEBEG

const int ARRAY_INITIAL_MAX_SIZE = 128; // bytes

template < typename T >
class Array
{
public:
    typedef T element;
    typedef T * iterator;

    Array<T>() : m_capacity_(0), m_item_count_(0), m_items_(0) {}
    explicit Array<T>(YETI_Cardinal count);
    Array<T>(YETI_Cardinal count, const T & item);
    Array<T>(const T * items, YETI_Cardinal item_count);
    Array<T>(const Array<T> & copy);
    ~Array<T>();

    Array<T> &  operator=(const Array<T> & copy);
    bool        operator==(const Array<T> & other) const;
    bool        operator!=(const Array<T> & other) const;
    T &         operator[](YETI_Cardinal pos) { return m_items_[pos]; }

    YETI_Cardinal   get_item_count() const { return m_item_count_; }
    YETI_Result     add(const T & item);
    YETI_Result     erase(iterator which);
    YETI_Result     erase(YETI_Cardinal which) { return erase(&m_items_[which]); }
    YETI_Result     erase(iterator first, iterator last);
    YETI_Result     erase(YETI_Cardinal first, YETI_Cardinal last) { return erase(&m_items_[first], &m_items_[last]); }
    YETI_Result     insert(iterator where, const T & item, YETI_Cardinal count = 1);
    YETI_Result     reserve(YETI_Cardinal count);
    YETI_Cardinal   get_capacity() const { return m_capacity_; }
    YETI_Result     resize(YETI_Cardinal count);
    YETI_Result     resize(YETI_Cardinal count, const T & fill);
    YETI_Result     clear();
    bool            contains(const T & data) const;
    iterator        get_first_item() const { return m_items_ ? &m_items_[0] : NULL; }
    iterator        get_last_item() const { return m_items_ ? &m_items_[m_item_count_ - 1] : NULL; }
    iterator        get_item(YETI_Ordinal n) { return n < m_item_count_ ? &m_items_[n] : NULL; }

    template < typename X >
    YETI_Result     apply(const X & function) const {
        for (unsigned int i = 0; i < m_item_count_; ++i) function(m_items_[i]);
        return YETI_SUCCESS;
    }

    template < typename X, typename P >
    YETI_Result     apply_until(const X & function, const P & predicate, bool * match = NULL) const {
        for (unsigned int i = 0; i < m_item_count_; ++i) {
            YETI_Result return_value;
            if (predicate(function(m_items_[i]), return_value)) {
                if (match) *match = true;
                return return_value;
            }
        }

        if (match) *match = false;
        return YETI_SUCCESS;
    }

    template < typename X >
    T * find(const X & predicate, YETI_Ordinal n = 0, YETI_Ordinal * pos = NULL) const {
        if (pos) *pos = -1;
        for (unsigned int i = 0; i < m_item_count_; ++i) {
            if (predicate(m_items_[i])) {
                if (pos) *pos = i;
                if (n == 0) return &m_items_[i];
                --n;
            }
        }

        return NULL;
    }

protected:
    T * allocate(YETI_Cardinal count, YETI_Cardinal & allocated);

    YETI_Cardinal m_capacity_;
    YETI_Cardinal m_item_count_;
    T *           m_items_;
};

template < typename T >
inline Array<T>::Array(YETI_Cardinal count)
: m_capacity_(0)
, m_item_count_(0)
, m_items_(0)
{
    reserve(count);
}

template < typename T >
inline Array<T>::Array(const Array<T> & copy)
: m_capacity_(0)
, m_item_count_(0)
, m_items_(0)
{
    reserve(copy.get_item_count());
    for (YETI_Ordinal i = 0; i < copy.m_item_count_; ++i) {
        new ((void *)&m_items_[i]) T(copy.m_items_[i]);
    }
    m_item_count_ = copy.m_item_count_;
}

template < typename T >
inline Array<T>::Array(YETI_Cardinal count, const T & item)
: m_capacity_(0)
, m_item_count_(count)
, m_items_(0)
{
    reserve(count);
    for (YETI_Ordinal i = 0; i < count; ++i) {
        new ((void *)&m_items_[i]) T(item);
    }
}

template < typename T >
inline Array<T>::Array( const T* item, YETI_Cardinal item_count)
: m_capacity_(0)
, m_item_count_(item_count)
, m_items_(0)
{
    reserve(item_count);
    for (YETI_Ordinal i = 0; i < item_count; ++i) {
        new ((void *)&item[i]) T(m_items_[i]);
    }
}

template < typename T >
inline Array<T>::~Array()
{
    clear();
    ::operator delete((void *)m_items_);
}

template < typename T >
Array<T> & Array<T>::operator =(const Array<T> & copy)
{
    if (this == &copy) return *this;

    clear();

    reserve(copy.get_item_count());
    m_item_count_ = copy.m_item_count_;
    for (YETI_Ordinal i = 0; i < copy.m_item_count_; ++i) {
        new ((void *)m_items_[i]) T(copy.m_items_[i]);
    }
    return *this;
}

template < typename T >
YETI_Result Array<T>::clear()
{
    for (YETI_Ordinal i = 0; i < m_item_count_; ++i) {
        m_items_[i].~T();
    }

    m_item_count_ = 0;
    return YETI_SUCCESS;
}

template < typename T >
T * Array<T>::allocate(YETI_Cardinal count, YETI_Cardinal & allocated)
{
    if (m_capacity_) {
        allocated = 2 * m_capacity_;
    } else {
        allocated = ARRAY_INITIAL_MAX_SIZE / sizeof(T);
        if (allocated == 0) allocated = 1;
    }

    if (allocated < count) allocated = count;
    return (T*)::operator new(allocated * sizeof(T));
}

template < typename T >
YETI_Result Array<T>::reserve(YETI_Cardinal count)
{
    if (count <= m_capacity_) return YETI_SUCCESS;
    YETI_Cardinal new_capacity;
    T * new_items = allocate(count, new_capacity);
    if (NULL == new_items) return YETI_ERROR_OUT_OF_MEMORY;
    if (m_item_count_ && m_items_) {
        for (unsigned int i = 0; i < m_item_count_; ++i) {
            new ((void *)&new_items[i]) T(m_items_[i]);
            m_items_[i].~T();
        }
    }
    ::operator delete((void *)m_items_);
    m_items_ = new_items;
    m_capacity_ = new_capacity;

    return YETI_SUCCESS;
}

template < typename T >
inline YETI_Result Array<T>::add(const T & item)
{
    YETI_Result result = reserve(m_item_count_ + 1);
    if (result != YETI_SUCCESS) return result;

    new ((void *)&m_items_[m_item_count_++]) T(item);

    return YETI_SUCCESS;
}

template < typename T >
inline YETI_Result Array<T>::erase(iterator which)
{
    return erase(which, which);
}

template < typename T >
YETI_Result Array<T>::erase(iterator first, iterator last)
{
    if (first == NULL || last == NULL) return YETI_ERROR_INVALID_PARAMETERS;

    YETI_Ordinal first_index = (YETI_Ordinal)(YETI_POINTER_TO_LONG(first - m_items_));
    YETI_Ordinal last_index = (YETI_Ordinal)(YETI_POINTER_TO_LONG(last - m_items_));

    if (first_index >= m_item_count_ || 
        last_index  >= m_item_count_ ||
        first_index > last_index) {
            return YETI_ERROR_INVALID_PARAMETERS;
    }

    YETI_Cardinal interval = last_index - first_index + 1;
    YETI_Cardinal shifted = m_item_count_ - last_index - 1;
    for (YETI_Ordinal i = first_index; i < first_index + shifted; ++i) {
        m_items_[i] = m_items_[i + interval];
    }

    for (YETI_Ordinal i = first_index + shifted; i < m_item_count_; ++i) {
        m_items_[i].~T();
    }

    m_item_count_ -= interval;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result Array<T>::insert(iterator where, const T & item, YETI_Cardinal repeat)
{
    YETI_Ordinal where_index = where ? ((YETI_Ordinal)YETI_POINTER_TO_LONG(where - m_items_)) : m_item_count_;
    if (where > &m_items_[m_item_count_] || repeat == 0) return YETI_ERROR_INVALID_PARAMETERS;

    YETI_Cardinal needed = m_item_count_ + repeat;
    if (needed > m_capacity_) {
        YETI_Cardinal new_capacity;
        T * new_items = allocate(needed, new_capacity);
        if (new_items == NULL) return YETI_ERROR_OUT_OF_MEMORY;
        m_capacity_ = new_capacity;

        for (YETI_Ordinal i = 0; i < where_index; ++i) {
            new((void *)&new_items[i]) T(m_items_[i]);
            m_items_[i].~T();
        }

        for (YETI_Ordinal i = where_index; i < m_item_count_; ++i) {
            new((void *)&new_items[i + repeat]) T(m_items_[i]);
            m_items_[i].~T();
        }

        ::operator delete((void *)m_items_);
        m_items_ = new_items;
    } else {
        for (YETI_Ordinal i = m_item_count_; i > where_index; --i) {
            new((void *)&m_items_[i + repeat - 1]) T(m_items_[i - 1]);
            m_items_[i - 1].~T();
        }
    }

    for (YETI_Cardinal i = where_index; i < where_index + repeat; ++i) {
        new((void *)&m_items_[i]) T(item);
    }

    m_item_count_ += repeat;

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result Array<T>::resize(YETI_Cardinal size)
{
    if (size < m_item_count_) {
        for (YETI_Ordinal i = size; i < m_item_count_; ++i) {
            m_items_[i].~T();
        }
        m_item_count_ = size;
    } else if (size > m_item_count_) {
        return resize(size, T());
    }

    return YETI_SUCCESS;
}

template < typename T >
YETI_Result Array<T>::resize(YETI_Cardinal size, const T & fill)
{
    if (size < m_item_count_) {
        return resize(size);
    } else if (size > m_item_count_) {
        reserve(size);
        for (YETI_Ordinal i = m_item_count_; i < size; ++i) {
            new ((void *)&m_items_[i]) T(fill);
        }
        m_item_count_ = size;
    }

    return YETI_SUCCESS;
}

template < typename T >
bool Array<T>::contains(const T & data) const
{
    for (YETI_Ordinal i = 0; i < m_item_count_; ++i) {
        if (m_items_[i] == data) return true;
    }

    return false;
}

template < typename T >
bool Array<T>::operator ==(const Array<T> & other) const
{
    if (other.m_item_count_ != m_item_count_) return false;

    for (YETI_Ordinal i = 0; i < m_item_count_; ++i) {
        if (!(m_items_[i] == other.m_items_[i])) return false;
    }

    return true;
}

template < typename T >
inline bool Array<T>::operator !=(const Array<T> & other) const
{
    return !(*this == other);
}

NAMEEND
 
#endif // _CXL_YETI_ARRAY_H_
