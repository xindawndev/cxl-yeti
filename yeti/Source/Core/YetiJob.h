#ifndef _CXL_YETI_JOB_H_
#define _CXL_YETI_JOB_H_

#include "YetiQueue.h"
#include "YetiArray.h"

NAMEBEG

class Job;

class JobCallback
{
public:
    virtual ~JobCallback() {};
    virtual void on_job_complete(
        unsigned int job_id, bool success, Job * job) = 0;
    virtual void on_job_progress(unsigned int job_id, unsigned int progress, unsigned int total, const Job * job) {}
};

class JobManager;

class Job
{
public:
    enum Priority {
        PRIORITY_LOW = 0,
        PRIORITY_NORMAL,
        PRIORITY_HIGH
    };
    Job() : m_callback_(NULL) {}
    virtual ~Job() {}
    virtual bool do_work() = 0;

    virtual const char * get_type() const { return ""; }
    virtual bool operator==(const Job * job) const {
        return false;
    }

    bool should_cancel(unsigned int progress, unsigned int total) const;

private:
    friend class JobManager;
    JobManager * m_callback_;
};

class JobManager
{
    class WorkItem
    {
    public:
        WorkItem(Job * job, unsigned int id, JobCallback * callback)
            : m_job_(job)
        , m_id_(id)
        , m_callback_(callback) {}

        bool operator==(unsigned int job_id) const {
            return m_id_ == job_id;
        }
        bool operator==(const Job * job) const {
            return m_job_ == job;
        }
        void free_job() {
            delete m_job_;
            m_job_ = NULL;
        }
        void cancel() {
            m_callback_ = NULL;
        }

        Job *           m_job_;
        unsigned int    m_id_;
        JobCallback *   m_callback_;
    };

public:
    static JobManager & get_instance();
    unsigned int add_job(Job * job, JobCallback * callback, Job::Priority priority = Job::PRIORITY_LOW);
    void cancel_job(unsigned int job_id);
    void cancel_jobs();
protected:
    friend class JobWorker;
    friend class Job;

    Job * get_next_job(const JobWorker * worker);
    void on_job_complete(bool success, Job * job);
    bool on_job_progress(unsigned int progress, unsigned int total, const Job * job) const;
private:
    JobManager();
    JobManager(const JobManager &);
    JobManager const & operator=(JobManager const &);
    virtual ~JobManager();
    Job * _pop_job();
    void _start_workers(Job::Priority priority);
    void _remove_worker(const JobWorker * worker);
    unsigned int _get_max_workers(Job::Priority priority) const;

    unsigned int m_job_counter_;

    typedef Queue<WorkItem> TypeJobQueue;
    typedef Array<WorkItem> TypeProcessing;
    typedef Array<JobWorker *> TypeWorkers;

    TypeJobQueue m_job_queue[Job::PRIORITY_HIGH + 1];
    TypeProcessing m_processing_;
    TypeWorkers m_workers_;

    bool m_running_;
};
NAMEEND

#endif // _CXL_YETI_JOB_H_
