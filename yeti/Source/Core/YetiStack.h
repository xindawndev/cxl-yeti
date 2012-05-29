#ifndef _CXL_YETI_STACK_H_
#define _CXL_YETI_STACK_H_

#include "YetiResults.h"
#include "YetiTypes.h"
#include "YetiList.h"

NAMEBEG

template < typename T >
class Stack : public List<T>
{
public:
    YETI_Result push(const T & value) {
        return this->add(value);
    }

    YETI_Result peek(T & value) {
        if (this->m_item_count_ == 0) return YETI_ERROR_NO_SUCH_ITEM;
        value = this->m_tail_->m_data_;
        return YETI_SUCCESS;
    }

    YETI_Result pop(T & value) {
        if (this->m_item_count_ == 0) return YETI_ERROR_NO_SUCH_ITEM;
        typename List<T>::iterator tail = this->get_last_item();
        value = *tail;
        return this->erase(tail);
    }
};

NAMEEND

#endif // _CXL_YETI_STACK_H_
