#ifndef _YETI_DLNA_YDENGINE_H_
#define _YETI_DLNA_YDENGINE_H_

#include "YDCommon.h"

namespace yeti
{
    namespace dlna
    {

        class IObject;

        class IListener
        {
        public:
            virtual void pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime) = 0;
            virtual void post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset) = 0;
            virtual void destroy(IObject * object) = 0;
        };

        class IObject : public IListener, public cxl::yeti::IObjAlloc
        {
        public:
            virtual void pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime) {}
            virtual void post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset) {}
            virtual void destroy(IObject * object) {}
        };

        class ThreadPool : public cxl::yeti::Singleton<ThreadPool>
        {
        public:
            ThreadPool();
            ~ThreadPool() {}

        public:
            static ThreadPool & get_singleton(void);
            static ThreadPool * get_singleton_ptr(void);

        public:
            typedef void (* thread_pool_handler)(void * var);

        public:
            void add_thread();
            void queue_user_work_item(void * var, thread_pool_handler callback);
            YETI_UInt32 get_thread_count() const { return m_thread_count_; }

        private:
            typedef struct ThreadPooWorkItem {
                thread_pool_handler callback;
                void * var;
            } TPWorkItem;

            YETI_UInt32 m_thread_count_;
            bool m_terminate_;
            cxl::yeti::List<TPWorkItem *> m_work_items_;
            cxl::yeti::Mutex m_sync_handle_;;
            cxl::yeti::Mutex m_abort_handle_;
            cxl::yeti::Mutex m_list_lock_;
        };

        class Engine : public cxl::yeti::Noncopyable, cxl::yeti::Singleton<Engine>
        {
        public:
            static Engine & get_singleton(void);
            static Engine * get_singleton_ptr(void);

        public:
            Engine();
            ~Engine() {}

        public:
            void reg_object(IObject * obj);
            void unreg_object(IObject * obj);

            void start(YETI_UInt32 threadnumber = 3);
            void stop();

        private:
            cxl::yeti::List<IObject *> m_objects_;
            bool m_started_;
        };
    }
}

#endif // _YETI_DLNA_YDENGINE_H_
