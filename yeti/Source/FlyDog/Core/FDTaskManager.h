#ifndef _FD_TASKMANAGER_H_
#define _FD_TASKMANAGER_H_

#include "FlyDog.h"

namespace flydog
{
    class ThreadTask;

    class TaskManager
    {
    public:
        TaskManager(YETI_Cardinal max_items = 0);
        virtual ~TaskManager();

        virtual YETI_Result start_task(ThreadTask * task, 
            TimeInterval * delay = NULL,
            bool auto_destroy = true);

        YETI_Result stop_all_tasks();

    private:
        friend class ThreadTask;

        YETI_Result add_task(ThreadTask * task);
        YETI_Result remove_task(ThreadTask * task);

    private:
        List<ThreadTask *>  m_tasks_;
        Mutex               m_tasks_lock_;
        Mutex               m_callback_lock_;
        Queue<int> *        m_queue_;
        YETI_Cardinal       m_max_tasks_;
        YETI_Cardinal       m_running_tasks_;
        bool                m_stopping_;
    };

}

#endif // _FD_TASKMANAGER_H_
