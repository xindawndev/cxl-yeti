#ifndef _YETI_DLNA_YDTIMER_H_
#define _YETI_DLNA_YDTIMER_H_

#include "YDCommon.h"
#include "YDEngine.h"

namespace yeti
{
    namespace dlna
    {
        typedef void (* timer_callback)(void *);

        class CallbackInfo
        {
        public:
            CallbackInfo(YETI_UInt32 milli_second, timer_callback callback, timer_callback destroy_callback = NULL)
                : m_milli_second_(milli_second)
                , m_callback_(callback)
                , m_destroy_callback_(destroy_callback)
            {
            }

            YETI_UInt32 milli_second() const {
                return m_milli_second_;
            }

            timer_callback callback() const {
                return m_callback_;
            }

            timer_callback destroy_callback() const {
                return m_destroy_callback_;
            }

        private:
            YETI_UInt32 m_milli_second_;
            timer_callback m_callback_;
            timer_callback m_destroy_callback_;
        };

        class Timer : public IObject
        {
        public:
            Timer(const Engine & engine);
            ~Timer() {}
        public:
            void add_timer(void * data, YETI_UInt32 milli_second, timer_callback tcb, timer_callback dtcb);
            virtual void pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime);
            virtual void post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset);
            virtual void destroy(IObject * object);
        private:
            const Engine & m_engine_;
            cxl::yeti::List<CallbackInfo> m_callbacks_info_;
        };

    }
}

#endif // _YETI_DLNA_YDTIMER_H_
