#ifndef _FD_THREADTASK_H_
#define _FD_THREADTASK_H_

#include "FlyDog.h"
#include "FDTaskManager.h"

namespace flydog
{
    class ThreadTask : public Runnable
    {
    public:
        friend class TaskManager;

        YETI_Result kill();

    protected:
        virtual bool is_aborting(YETI_Timeout timeout) {
            return YETI_SUCCESS(m_abort_.wait_until_equals(1, timeout));
        }

        YETI_Result start(TaskManager * task_manager = NULL, 
            TimeInterval * delay = NULL,
            bool auto_destroy = true);
        YETI_Result stop(bool blocking = true);

        virtual void do_init()    {}
        virtual void do_abort()   {}
        virtual void do_run()     {}

        ThreadTask();

        virtual ~ThreadTask();

    private:
        YETI_Result start_thread();

        void run();

    protected:
        TaskManager*    m_task_manager_;

    private:
        SharedVariable  m_started_;
        SharedVariable  m_abort_;
        Thread*         m_thread_;
        bool            m_auto_destroy_;
        TimeInterval    m_delay_;
    };

}

#endif // _FD_THREADTASK_H_
