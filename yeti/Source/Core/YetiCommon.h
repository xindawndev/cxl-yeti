#ifndef _CXL_YETI_COMMON_H_
#define _CXL_YETI_COMMON_H_

#include "YetiTypes.h"
#include "YetiResults.h"

NAMEBEG

template < class T >
class ObjectDeleter {
public:
    void operator()(T *& object) const {
        if (NULL != object) {
            delete object;
            object = NULL;
        }
    }
};

template < class T >
class ObjectComparator {
public:
    ObjectComparator(T & object) : m_object_(object) {}
    bool operator()(const T & object) const {
        return object == m_object_;
    }

private:
    T & m_object_;
};

template < typename T, typename P >
YETI_Result container_find(T &                      container,
                           const P &                predicate,
                           typename T::element &    item,
                           YETI_Ordinal             n = 0)
{
    typename T::iterator found = container.find(predicate, n);
    if (found) {
        item = *found;
        return YETI_SUCCESS;
    } else {
        return YETI_ERROR_NO_SUCH_ITEM;
    }
}

template < typename T, typename P >
YETI_Result container_find(T &                  container,
                           const P &            predicate,
                           typename T::iterator &iter,
                           YETI_Ordinal         n = 0)
{
    iter = container.find(predicate, n);
    return iter ? YETI_SUCCESS : YETI_ERROR_NO_SUCH_ITEM;
}

class UnitlResultEquals
{
public:
    UnitlResultEquals(YETI_Result condition_result,
        YETI_Result return_value = YETI_SUCCESS)
        : m_condition_result_(condition_result)
        , m_return_value_(return_value)
    {}

    bool operator()(YETI_Result result, YETI_Result & return_value) const {
        if (result == m_condition_result_) {
            return_value = m_return_value_;
            return true;
        } else {
            return false;
        }
    }

private:
    YETI_Result m_condition_result_;
    YETI_Result m_return_value_;
};

class UnitlResultNotEquals
{
public:
    UnitlResultNotEquals(YETI_Result condition_result)
        : m_condition_result_(condition_result)
    {}

    bool operator()(YETI_Result result, YETI_Result & return_value) const {
        if (result != m_condition_result_) {
            return_value = result;
            return true;
        } else {
            return false;
        }
    }

private:
    YETI_Result m_condition_result_;
};

class PropertyValue
{
public:
    typedef enum {UNKNOWN, INTEGER, STRING} Type;

    PropertyValue()                     : m_type_(UNKNOWN), m_integer_(0)       {}
    PropertyValue(int value)            : m_type_(INTEGER), m_integer_(value)   {}
    PropertyValue(const char * value)   : m_type_(STRING), m_string_(value)     {}

    Type m_type_;
    union {
        int         m_integer_;
        const char * m_string_;
    };
};

NAMEEND

#endif // _CXL_YETI_COMMON_H_
