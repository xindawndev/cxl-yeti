#include "YetiThreads.h"

NAMEBEG

Mutex SingletonLock::instance_;

ThreadCallbackSlot::ThreadCallbackSlot()
: m_callback_args_(NULL)
, m_shutdown_(false)
, m_notification_helper_(NULL)
{
}

YETI_Result ThreadCallbackSlot::shutdown()
{
    AutoLock lock(m_read_lock_);

    m_shutdown_ = true;

    m_pending_.set_value(0);
    m_ack_.set_value(1);

    return YETI_SUCCESS;
}

YETI_Result ThreadCallbackSlot::set_notification_helper(NotificationHelper * helper)
{
    m_notification_helper_ = helper;
    return YETI_SUCCESS;
}

YETI_Result ThreadCallbackSlot::receive_callback(ThreadCallbackReceiver & receiver, YETI_Timeout timeout /* = 0 */)
{
    AutoLock lock(m_read_lock_);

    if (timeout) {
        YETI_Result result = m_pending_.wait_until_equals(1, timeout);
        if (YETI_FAILED(result)) return result;
    } else {
        if (m_pending_.get_value() == 0) {
            return YETI_ERROR_CALLBACK_NOTHING_PENDING;
        }
    }

    if (m_shutdown_) return YETI_ERROR_CALLBACK_HANDLER_SHUTDOWN;
    receiver.on_callback(const_cast<void *>(m_callback_args_));

    m_pending_.set_value(0);
    m_ack_.set_value(1);

    return YETI_SUCCESS;
}

YETI_Result ThreadCallbackSlot::send_callback(void * args)
{
    AutoLock lock(m_write_lock_);
#if defined(YETI_DEBUG)
    YETI_ASSERT(m_pending_.get_value() == 0);
#endif
    if (m_shutdown_) return YETI_ERROR_CALLBACK_HANDLER_SHUTDOWN;

    m_callback_args_ = args;
    m_pending_.set_value(1);
    if (m_notification_helper_) {
        m_notification_helper_->notify();
    }

    m_ack_.wait_until_equals(1);
    m_ack_.set_value(0);
    m_callback_args_ = NULL;

    return m_shutdown_ ? YETI_ERROR_CALLBACK_HANDLER_SHUTDOWN : YETI_SUCCESS;
}

NAMEEND
