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

        class IObject : public IListener
        {
        public:
            virtual void pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime) {}
            virtual void post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset) {}
            virtual void destroy(IObject * object) {}
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

            void start();
            void stop();

        private:
            cxl::yeti::List<IObject *> m_objects_;
            bool m_started_;
        };
    }
}

#endif // _YETI_DLNA_YDENGINE_H_
