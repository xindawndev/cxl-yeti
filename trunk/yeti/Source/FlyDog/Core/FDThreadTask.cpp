
#include "FDThreadTask.h"
#include "FDTaskManager.h" 

YETI_SET_LOCAL_LOGGER("flydog.core.threadtask")

namespace flydog
{
    ThreadTask::ThreadTask()
        : m_task_manager_(NULL),
        m_thread_(NULL),
        m_auto_destroy_(false)
    {
    }

    ThreadTask::~ThreadTask()
    {
        if (!m_auto_destroy_) delete m_thread_;
    }

    YETI_Result ThreadTask::start(TaskManager*  task_manager,/* = NULL */
        TimeInterval* delay, /* = NULL */
        bool auto_destroy /* = true */)
    {
        m_abort_.set_value(0);
        m_auto_destroy_ = auto_destroy;
        m_delay_        = delay ? *delay : TimeStamp(0.);
        m_task_manager_ = task_manager;

        if (m_task_manager_) {
            YETI_CHECK_SEVERE(m_task_manager_->add_task(this));
            return YETI_SUCCESS;
        } else {
            return start_thread();
        }
    }

    YETI_Result ThreadTask::start_thread()
    {
        m_started_.set_value(0);

        m_thread_ = new Thread((Runnable&)*this, m_auto_destroy_);
        YETI_CHECK_SEVERE(m_thread_->start());

        return m_started_.wait_until_equals(1, YETI_TIMEOUT_INFINITE);
    }

    YETI_Result ThreadTask::stop(bool blocking /* = true */)
    {
        bool auto_destroy = m_auto_destroy_;

        m_abort_.set_value(1);
        do_abort();

        if (!blocking || !m_thread_) return YETI_SUCCESS;

        return auto_destroy ? YETI_FAILURE : m_thread_->wait();
    }

    YETI_Result ThreadTask::kill()
    {
        stop();

        YETI_ASSERT(m_auto_destroy_ == false);
        if (!m_auto_destroy_) delete this;

        return YETI_SUCCESS;
    }

    void ThreadTask::run() 
    {
        m_started_.set_value(1);

        if ((float)m_delay_ > 0.f) {
            if ((float)m_delay_ > 0.1f) {
                TimeStamp start, now;
                System::get_current_timestamp(start);
                do {
                    System::get_current_timestamp(now);
                    if (now >= start + m_delay_) break;
                } while (!is_aborting(100));
            } else {
                System::sleep(m_delay_);
            }
        }

        if (!is_aborting(0))  {
            do_init();
            do_run();
        }

        if (m_task_manager_) {
            m_task_manager_->remove_task(this);
        } else if (m_auto_destroy_) {
            delete this;
        }
    }

}
