#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#include "YetiConfig.h"
#include "YetiTypes.h"
#include "YetiQueue.h"
#include "YetiThreads.h"
#include "YetiList.h"
#include "YetiDebug.h"
#include "YetiWin32Thread.h"
#include "YetiLogging.h"

//YETI_SET_LOCAL_LOGGER("yeti.queue.win32")

NAMEBEG

class Win32Queue : public GenericQueue
{
public:
    Win32Queue(YETI_Cardinal max_items)
        : m_max_items_(max_items) {
            m_can_push_condition_ = new Win32Event(true, true);
            m_can_pop_condition_  = new Win32Event(true, false);
    }
    ~Win32Queue() {
        delete m_can_pop_condition_;
        m_can_pop_condition_ = NULL;
        delete m_can_push_condition_;
        m_can_push_condition_ = NULL;
    }

    YETI_Result push(QueueItem * item,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
    YETI_Result pop(QueueItem *& item,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);
    YETI_Result peek(QueueItem *& item,
        YETI_Timeout timeout = YETI_TIMEOUT_INFINITE);

private:
    YETI_Cardinal        m_max_items_;
    Win32CriticalSection m_mutex_;
    Win32Event *         m_can_push_condition_;
    Win32Event *         m_can_pop_condition_;
    List<QueueItem *>    m_items_; // should be volatile ?
};

YETI_Result Win32Queue::push(QueueItem * item, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    YETI_CHECK(m_mutex_.lock());

    if (m_max_items_) {
        while (m_items_.get_item_count() >= m_max_items_) {
            m_can_push_condition_->reset();
            m_mutex_.unlock();
            YETI_Result result = m_can_push_condition_->wait(timeout);
            if (YETI_FAILED(result)) return result;
            YETI_CHECK(m_mutex_.lock());
        }
    }

    m_items_.add(item);
    m_can_pop_condition_->signal();
    m_mutex_.unlock();
    return YETI_SUCCESS;
}

YETI_Result Win32Queue::pop(QueueItem *& item, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    item = NULL;
    YETI_CHECK(m_mutex_.lock());
    YETI_Result result;
    if (timeout) {
        while ((result = m_items_.pop_head(item)) == YETI_ERROR_LIST_EMPTY) {
            m_can_pop_condition_->reset();
            m_mutex_.unlock();
            YETI_Result result = m_can_pop_condition_->wait(timeout);
            if (YETI_FAILED(result)) return result;
            YETI_CHECK(m_mutex_.lock());
        }
    } else {
        result = m_items_.pop_head(item);
    }

    if (m_max_items_ && (result == YETI_SUCCESS)) {
        m_can_push_condition_->signal();
    }
    m_mutex_.unlock();
    return result;
}

YETI_Result Win32Queue::peek(QueueItem *& item, YETI_Timeout timeout /* = YETI_TIMEOUT_INFINITE */)
{
    item = NULL;
    YETI_CHECK(m_mutex_.lock());
    YETI_Result result = YETI_SUCCESS;
    List<QueueItem *>::iterator head = m_items_.get_first_item();
    if (timeout) {
        while (!head) {
            m_can_pop_condition_->reset();
            m_mutex_.unlock();
            YETI_Result result = m_can_pop_condition_->wait(timeout);
            if (YETI_FAILED(result)) return result;
            YETI_CHECK(m_mutex_.lock());
            head = m_items_.get_first_item();
        }
    } else {
        if (!head) result = YETI_ERROR_LIST_EMPTY;
    }
    if (head) item = *head;
    m_mutex_.unlock();
    return result;
}

GenericQueue * GenericQueue::create_instance(YETI_Cardinal max_items /* = 0 */)
{
    return new Win32Queue(max_items);
}

NAMEEND
