#ifndef __ASYNCTASK_H__
#define __ASYNCTASK_H__

#include "Common.h"

#include <boost/asio.hpp>
#include <boost/signal.hpp>

const int SINGLE_THREAD_POOL_SIZE = 1;

class CAsyncTask
    : public boost::enable_shared_from_this<CAsyncTask>
{
public:
    CAsyncTask(int threadPoolSize);
    virtual ~CAsyncTask(); 

public:    
    // Start async task
    virtual bool Start();
    // Stop async task and cancel all uncompleted handler ASAP
    virtual void Stop();

    class CQueuedHandler {
    public:
        CQueuedHandler(
            int p,
            boost::function<void()> f)
                : m_priority(p)
                , m_function(f) {
        }
    
        CQueuedHandler() {
        }

        void Execute() {
            m_function();
        }

        friend bool operator<(
            const CQueuedHandler& a,
            const CQueuedHandler& b) {
            return a.m_priority < b.m_priority;
        }

        int m_priority;
        boost::function<void()> m_function;
    };

    // Post method
    template <typename Handler>
    void Post(
        Handler handler,
        int priority = 0) {
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_is_stopped) {
            return;
        }

        // Add handler
        boost::shared_ptr<CQueuedHandler> queuedHandler(new CQueuedHandler(priority, handler));
        m_queue.push(queuedHandler);
        m_ioService->post(
            boost::bind(&CAsyncTask::ExecHandler,
            shared_from_this()));
        return;
    }

protected:
    void ExecHandler();

protected:
    mutable boost::mutex m_mutex;
    volatile bool m_is_stopped;
    boost::shared_ptr<boost::asio::io_service> m_ioService;
    boost::scoped_ptr<boost::asio::io_service::work> m_idleWork;
    int m_threadpool_size;
    std::vector<boost::shared_ptr<boost::thread> > m_work_threadpool;
    //std::priority_queue<boost::shared_ptr<CQueuedHandler> > m_priQueue; // priority queue for handler
    std::queue<boost::shared_ptr<CQueuedHandler> > m_queue; // queue for handler
};

#endif // __ASYNCTASK_H__
