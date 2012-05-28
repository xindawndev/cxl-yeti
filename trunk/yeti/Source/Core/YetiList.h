#ifndef _CXL_YETI_LIST_H_
#define _CXL_YETI_LIST_H_

#include "YetiResults.h"
#include "YetiTypes.h"
#include "YetiConstants.h"
#include "YetiCommon.h"

NAMEBEG

const int YETI_ERROR_LIST_EMPTY                 = YETI_ERROR_BASE_LIST - 0;
const int YETI_ERROR_LIST_OPERATION_ABORTED     = YETI_ERROR_BASE_LIST - 1;
const int YETI_ERROR_LIST_OPERATION_CONTINUE    = YETI_ERROR_BASE_LIST - 2;

template < typename T >
class List
{
protected:
    class Item;

public:
    typedef T element;

    class iterator
    {
    public:
        iterator() : m_item_(NULL) {}
        explicit iterator(Item * item) : m_item_(item) {}
        iterator(const iterator & copy) : m_item_(copy.m_item_) {}

        T & operator*() const { return m_item_->m_data_; }
        T * operator->() const {return &m_item_->m_data_; }

        iterator & operator++() {
            m_item_ = m_item_->n_next_;
            return (*this);
        }

        iterator & operator++(int) {
            iterator saved_this = *this;
            m_item_ = m_item_->m_next_;
            return saved_this;
        }

    private:
        Item * m_item_;

        friend class List<T>;
    };
};

NAMEEND

#endif // _CXL_YETI_LIST_H_
