#include "FDTaskManager.h"
#include "FDThreadTask.h"

YETI_SET_LOCAL_LOGGER("flydog.core.taskmanager")

namespace flydog
{
    TaskManager::TaskManager(YETI_Cardinal max_items /* = 0 */)
        : m_queue_(NULL),
        m_max_tasks_(max_items),
        m_running_tasks_(0),
        m_stopping_(false)
    {
    }

    TaskManager::~TaskManager()
    {    
        stop_all_tasks();
    }

    YETI_Result TaskManager::start_task(ThreadTask * task, 
        TimeInterval * delay /* = NULL*/,
        bool auto_destroy /* = true */)
    {
        YETI_CHECK_POINTER_SEVERE(task);
        return task->start(this, delay, auto_destroy);
    }

    YETI_Result TaskManager::stop_all_tasks()
    {
        {
            AutoLock lock(m_tasks_lock_);

            m_stopping_ = true;

            if (m_queue_) {
                Queue<int>* queue = m_queue_;
                m_queue_ = NULL;
                delete queue;
            }  

            List<ThreadTask *>::iterator task = m_tasks_.get_first_item();
            while (task) {
                (*task)->Stop(false);
                ++task;
            }
        }

        YETI_Cardinal num_running_tasks;
        do {
            {
                AutoLock lock(m_tasks_lock_);
                num_running_tasks = m_tasks_.get_item_count();
            }

            if (num_running_tasks == 0) 
                break; 

            System::sleep(TimeInterval(0.05));
        } while (1);

        m_stopping_ = false;
        return YETI_SUCCESS;
    }

    YETI_Result TaskManager::add_task(ThreadTask* task) 
    {
        AutoLock lock(m_tasks_lock_);
        if (m_stopping_) YETI_CHECK_SEVERE(YETI_ERROR_INVALID_STATE);

        if (!m_queue_ && m_max_tasks_) {
            m_queue_ = new Queue<int>(m_max_tasks_);
        }

        if (m_queue_) YETI_CHECK_SEVERE(m_queue_->Push(new int));

        YETI_LOG_FINER_3("[TaskManager 0x%08x] %d/%d running tasks", this, ++m_running_tasks_, m_max_tasks_);
        YETI_CHECK_SEVERE(task->start_thread());
        return m_tasks_.Add(task);
    }

    YETI_Result TaskManager::remove_task(ThreadTask* task)
    {
        {
            AutoLock lock(m_tasks_lock_);

            if (m_queue_) {
                int* val = NULL;
                if (YETI_SUCCEEDED(m_queue_->pop(val)))
                    delete val;
            }

            YETI_LOG_FINER_3("[TaskManager 0x%08x] %d/%d running tasks", this, --m_running_tasks_, m_max_tasks_);
            m_tasks_.remove(task);
        }

        if (task->m_auto_destroy_) delete task;

        return YETI_SUCCESS;
    }

}
