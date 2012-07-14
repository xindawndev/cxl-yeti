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
