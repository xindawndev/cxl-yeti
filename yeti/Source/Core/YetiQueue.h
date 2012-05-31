#ifndef _CXL_YETI_QUEUE_H_
#define _CXL_YETI_QUEUE_H_

#include "YetiTypes.h"
#include "YetiConstants.h"

NAMEBEG

class QueueItem;
class GenericQueue
{
public:
    static GenericQueue * create_instance(YETI_Cardinal max_items = 0);

    virtual             ~GenericQueue() {}
    virtual YETI_Result push(QueueItem * item,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) = 0;
    virtual YETI_Result pop(QueueItem *& item,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) = 0;
    virtual YETI_Result peek(QueueItem *& item,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) = 0;

protected:
    GenericQueue() {}
};

template < class T >
class Queue
{
public:
    Queue(YETI_Cardinal max_items = 0) 
        : m_delegate_(GenericQueue::create_instance(max_items)) {}
    virtual ~Queue<T>() { delete m_delegate_; }

    virtual YETI_Result push(T * item, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_delegate_->push(reinterpret_cast<QueueItem *>(item), timeout);
    }
    virtual YETI_Result pop(T *& item, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_delegate_->pop(reinterpret_cast<QueueItem *&>(item), timeout);
    }
    virtual YETI_Result peek(T *& item, YETI_Timeout timeout = YETI_TIMEOUT_INFINITE) {
        return m_delegate_->peek(reinterpret_cast<QueueItem *&>(item), timeout);
    }

protected:
    GenericQueue * m_delegate_;
};

NAMEEND

#endif // _CXL_YETI_QUEUE_H_
