#include "YDTimer.h"

namespace yeti
{
    namespace dlna
    {
        Timer::Timer(const Engine & engine)
            : m_engine_(engine)
        {
            Engine::get_singleton().reg_object(this);
        }

        YETI_Result Timer::add_event(void * data, YETI_UInt32 milli_second, timer_callback tcb, timer_callback dtcb)
        {
            if (milli_second < 0 || tcb == NULL) return YETI_ERROR_INVALID_PARAMETERS;

            CallbackInfo cbi(data, milli_second, tcb, dtcb);
            m_callbacks_info_.add(cbi);

            return YETI_SUCCESS;
        }

        YETI_Result Timer::remove_event(void * data)
        {
            return YETI_SUCCESS;
        }

        void Timer::pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime)
        {

        }

        void Timer::post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset)
        {

        }

        void Timer::destroy(IObject * object)
        {

        }
    }
}
