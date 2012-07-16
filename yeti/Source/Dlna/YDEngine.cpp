#include "YDEngine.h"

#if (defined(_WIN32) || defined(_WIN32_WCE) || defined(_XBOX)) && !defined(__SYMBIAN32__)
#   if !defined(__WINSOCK__)
#       define __WINSOCK__
#   endif
#endif

#if defined(__WINSOCK__) &&!defined(_XBOX)
#   define STRICT
#   define YETI_WIN32_USE_WINSOCK2
#   ifdef YETI_WIN32_USE_WINSOCK2
#       include <winsock2.h>
#       include <ws2tcpip.h>
#   else
#       include <winsock.h>
#   endif
#   include <windows.h>
#   include "YetiConfig.h"
#   include "YetiWin32Network.h"
NAMEBEG
static WinsockSystem & WinsockInitializer = WinsockSystem::Initializer;
NAMEEND
#else
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <netdb.h>
#   include <errno.h>
#endif

USINGNAMESPACE2;

template <> yeti::dlna::Engine * Singleton<yeti::dlna::Engine>::m_singleton_ = NULL;
template <> yeti::dlna::ThreadPool * Singleton<yeti::dlna::ThreadPool>::m_singleton_ = NULL;

namespace yeti
{
    namespace dlna
    {
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
            : m_started_(false)
        {

        }

        void Engine::reg_object(IObject * obj)
        {

        }

        void Engine::unreg_object(IObject * obj)
        {

        }

        void Engine::start(YETI_UInt32 threadnumber)
        {
            stop();
            for (YETI_UInt32 i = 0; i < threadnumber; ++i) {
            }

            m_started_ = true;
            while (m_started_) {
                List<IObject *>::iterator it;
                for (it = m_objects_.get_first_item(); it; ++it) {
                    //(*it)->pre_process(*it, );
                }
                //select();
                for (it = m_objects_.get_first_item(); it; ++it) {
                    //(*it)->post_process(*it, );
                }
            }
        }

        void Engine::stop()
        {
            m_started_ = false;
            List<IObject *>::iterator it;
            for (it = m_objects_.get_first_item(); it; ++it) {
                (*it)->destroy(*it);
            }
            m_objects_.clear();
        }
    }
}
