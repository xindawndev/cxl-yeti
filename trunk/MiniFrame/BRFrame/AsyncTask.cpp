#include "AsyncTask.h"

const int MAX_JOIN_TIME = 100; // ms

CAsyncTask::CAsyncTask(
                       int threadpool_size)
                       : m_threadpool_size(threadpool_size)
                       , m_is_stopped(true)
{
}

CAsyncTask::~CAsyncTask()
{
}

bool CAsyncTask::Start() 
{
    boost::mutex::scoped_lock lock(m_mutex);
    if(!m_is_stopped) { // Already start
        return false;
    }

    m_ioService.reset(new boost::asio::io_service());
    m_idleWork.reset(new boost::asio::io_service::work(*m_ioService.get()));

    for (int i = 0; i < m_threadpool_size; i++) {
        boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, m_ioService)));
        m_work_threadpool.push_back(thread);
    }

    // Set status as started in final step
    m_is_stopped = false;

    return true;
}

void CAsyncTask::Stop() 
{
    std::vector<boost::shared_ptr<boost::thread> > work_threadpool;

    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (m_is_stopped) { // Already stop
            return;
        }

        // Set status as stopped first
        m_is_stopped = true;

        try {
            // Clear priority queue by assigning a empty queue since priority queue doesn't have clear() method
            //m_priQueue = std::priority_queue<boost::shared_ptr<CQueuedHandler> >(); 
            m_queue = std::queue<boost::shared_ptr<CQueuedHandler> >(); 
            m_ioService->stop();
            m_idleWork.reset();
        } catch (...) {
        }

        work_threadpool = m_work_threadpool;
        m_work_threadpool.clear();
    }

    // Terminate threads
    std::vector<boost::shared_ptr<boost::thread> > ::iterator iter = work_threadpool.begin();
    while (iter != work_threadpool.end()) {
        boost::thread::id pid = (*iter)->get_id();
        (*iter)->interrupt();
        (*iter)->join();

        iter = work_threadpool.erase(iter);
    }
    return;
}

void CAsyncTask::ExecHandler()
{
    boost::shared_ptr<CQueuedHandler> queue_handler; 

    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (m_is_stopped) {
            return;
        }

        if (!m_queue.empty()) {
            //queuedHandler = m_priQueue.top();
            //m_priQueue.pop();
            queue_handler = m_queue.front();
            m_queue.pop();
        }
    }

    if (queue_handler.get()) {
        queue_handler->Execute();
    }
}
