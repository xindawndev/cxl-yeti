#ifndef _CXL_YETI_JOBMANAGER_H_
#define _CXL_YETI_JOBMANAGER_H_

#include "YetiJob.h"
#include "YetiString.h"
#include "YetiThreads.h"

NAMEBEG

class JobManager;

class JobWorker : public Thread
{
public:
    JobWorker(JobManager * manager);
    virtual ~JobWorker();

    void run();

private:
    JobManager * m_job_manager_;
};

class JobQueue : public JobCallback
{
    class JobPointer
    {
    public:
        JobPointer(Job * job) {
            m_job_ = job;
            m_id_ = 0;
        }
        void cancel_job();
        void free_job() {
            delete m_job_;
            m_job_ = NULL;
        }
        bool operator==(const Job * job) const {
            if (m_job_) return *m_job_ == job;
            return false;
        }
        Job * m_job_;
        unsigned int m_id_;
    };
public:
    JobQueue(bool lifo = false, unsigned int job_at_once = 1, Job::Priority priority = Job::PRIORITY_LOW);
    virtual ~JobQueue();
    void add_job(Job * job);
    void cancel_jobs();
    virtual void on_job_complete(unsigned int job_id, bool success, Job * job);

private:
    void _queue_next_job();
    typedef Queue<JobPointer> type_queue;
    typedef Array<JobPointer> type_processing;
    type_queue m_job_queue_;
    type_processing m_processing_;
    unsigned int m_job_at_once_;
    Job::Priority m_priority_;

    Mutex m_section_;
    bool m_lifo_;
};


NAMEEND

#endif // _CXL_YETI_JOBMANAGER_H_
