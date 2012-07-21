#include "YDEngine.h"

#if defined(__WINSOCK__) &&!defined(_XBOX)
#   include "YetiWin32Network.h"
NAMEBEG
static WinsockSystem & WinsockInitializer = WinsockSystem::Initializer;
NAMEEND
#endif

#include <time.h>

USINGNAMESPACE2;

template <> yeti::dlna::Engine * Singleton<yeti::dlna::Engine>::m_singleton_ = NULL;
template <> yeti::dlna::ThreadPool * Singleton<yeti::dlna::ThreadPool>::m_singleton_ = NULL;
YETI_UInt32 yeti::dlna::Engine::ENGINE_MAX_WAIT = 86400;

namespace yeti
{
    namespace dlna
    {

        class PoolThread : public Thread
        {
        public:
            void run() {
                yeti_debug("thread begin\n");
                ThreadPool::get_singleton().add_thread();
            }
        };

        ThreadPool & ThreadPool::get_singleton(void) {
            YETI_ASSERT(m_singleton_);
            return (*m_singleton_);
        }

        ThreadPool * ThreadPool::get_singleton_ptr(void) {
            return m_singleton_;
        }

        ThreadPool::ThreadPool()
            : m_thread_count_(0)
        {

        }

        void ThreadPool::add_thread()
        {
            TPWorkItem * work_item = NULL;
            bool abort = false;
            int id;
            int ok = 0;

            m_list_lock_.lock();
            ++m_thread_count_;
            id = m_thread_count_;
            m_list_lock_.unlock();

            do {
                m_sync_handle_.lock();

                m_list_lock_.lock();
                YETI_Result ret = m_work_items_.pop_head(work_item);
                abort = m_terminate_;
                m_list_lock_.unlock();

                if(ret != YETI_ERROR_LIST_EMPTY && work_item != NULL && !abort){
                    work_item->callback(work_item->var);
                    delete work_item;
                } else if (work_item != NULL) {
                    delete work_item;
                }

                m_list_lock_.lock();
                abort = m_terminate_;
                m_list_lock_.unlock();

            } while( !abort );

            m_list_lock_.lock();
            --m_thread_count_;
            ok = m_thread_count_;
            m_list_lock_.unlock();

            if(ok == 0) {
                m_abort_handle_.unlock();
            }
        }

        void ThreadPool::queue_user_work_item(void * var, thread_pool_handler callback)
        {
            TPWorkItem * wi = NULL;
            YETI_UInt32 num_threads = 0;

            wi = new TPWorkItem;
            wi->var = var;
            wi->callback = callback;

            m_list_lock_.lock();
            if (m_thread_count_ != 0) {
                num_threads = m_thread_count_;
                m_work_items_.add(wi);
                m_sync_handle_.unlock();
            }
            m_list_lock_.unlock();

            if (num_threads == 0) {
                wi->callback(wi->var);
                delete wi;
            }
        }

        //---------------------------------------------------------------------
        // Engine
        Engine & Engine::get_singleton(void) {
            YETI_ASSERT(m_singleton_);
            return (*m_singleton_);
        }

        Engine * Engine::get_singleton_ptr(void) {
            return m_singleton_;
        }

        Engine::Engine()
            : m_running_flag_(false)
            , m_terminate_flag_(false)
        {
            srand((unsigned int)time(NULL));
#if defined(WIN32) || defined(_WIN32_WCE)
            m_terminate_ = socket(AF_INET, SOCK_DGRAM, 0);
#endif
        }

        void Engine::reg_object(IObject * obj)
        {
            AutoLock autolock(m_obj_mutex_);
            m_objects_.add(obj);
        }

        void Engine::unreg_object(IObject * obj)
        {
            AutoLock autolock(m_obj_mutex_);
            //m_objects_.detach(obj);
        }

        void Engine::start(YETI_UInt32 threadnumber)
        {
            for (YETI_UInt32 i = 0; i < threadnumber; ++i) {
                Thread * thread = new PoolThread;
                m_threads_.add(thread);
                thread->start();
            }

            fd_set readset;
            fd_set writeset;
            fd_set errorset;

            FD_ZERO(&readset);
            FD_ZERO(&writeset);
            FD_ZERO(&errorset);

            struct timeval tv;
            int slct;
            int v;

#if !defined(_WIN32)
            int terminate_pipe[2];
            int flags;
#endif

            srand((unsigned int)time(NULL));

#if !defined(_WIN32)
            pipe(terminate_pipe);
            flags = fcntl(terminate_pipe[0], F_GETFL, 0);
            fcntl(terminate_pipe[0], F_SETFL, 0, _NONBLOCK | flags);

#endif
            m_running_flag_ = true;
            m_terminate_flag_ = false;
            while (m_terminate_flag_ == false) {
                slct = 0;
                FD_ZERO(&readset);
                FD_ZERO(&errorset);
                FD_ZERO(&writeset);
                tv.tv_sec = ENGINE_MAX_WAIT;
                tv.tv_usec = 0;

                List<IObject *>::iterator it;
                for (it = m_objects_.get_first_item(); it; ++it) {
                    v = (tv.tv_sec*1000) + (tv.tv_usec/1000);
                    (*it)->pre_process(*it, &readset, &writeset, &errorset, &v);
                    tv.tv_sec = v/1000;
                    tv.tv_usec = 1000*(v%1000);
                }

                m_obj_mutex_.lock();
#if defined(WIN32) || defined(_WIN32_WCE)
                if(m_terminate_ == ~0) {
                    slct = -1;
                } else {
                    FD_SET(m_terminate_, &errorset);
                }
#else
                FD_SET(terminate_pipe[0], &readset);
#endif
                m_obj_mutex_.unlock();

                if (slct != 0) {
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;
                }

                slct = select(FD_SETSIZE, &readset, &writeset, &errorset, &tv);

                if (slct == -1) {
                    FD_ZERO(&readset);
                    FD_ZERO(&writeset);
                    FD_ZERO(&errorset);
                }
#if defined(WIN32) || defined(_WIN32_WCE)
                if (m_terminate_ == ~0) {
                    m_terminate_ = socket(AF_INET, SOCK_DGRAM, 0);
                }
#else
                if (FD_ISSET(terminate_pipe[0], &readset)) {
                    while(fgetc(m_terminate_readpipe_) != EOF);
                }
#endif
                for (it = m_objects_.get_first_item(); it; ++it) {
                    (*it)->post_process(*it, slct, &readset, &writeset, &errorset);
                }
            }

            List<IObject *>::iterator iter;
            for (iter = m_objects_.get_first_item(); iter; ++iter) {
                (*iter)->destroy(*iter);
            }

            m_objects_.clear();

#if defined(WIN32)
            if (m_terminate_ != ~0) {
                closesocket(m_terminate_);
                m_terminate_ = ~0;
            }
#else
            fclose(m_terminate_readpipe_);
            fclose(m_terminate_writepipe_);
            m_terminate_readpipe_ = 0;
            m_terminate_writepipe_ = 0;
#endif
        }

        void Engine::stop()
        {
            m_running_flag_ = false;
            m_terminate_flag_ = true;
            List<IObject *>::iterator it;
            for (it = m_objects_.get_first_item(); it; ++it) {
                (*it)->destroy(*it);
            }
            m_objects_.clear();
            List<Thread *>::iterator tit;
            for (tit = m_threads_.get_first_item(); tit; ++tit) {
                (*tit)->wait();
                delete (*tit);
            }
            m_threads_.clear();
        }
    }
}
