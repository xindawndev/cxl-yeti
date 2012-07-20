#include "YDAsyncSocket.h"

namespace yeti
{
    namespace dlna
    {
        AsyncSocket::AsyncSocket(const Engine & engine)
            : m_engine_(engine)
        {
            Engine::get_singleton().reg_object(this);
        }

        void AsyncSocket::pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime)
        {

        }

        void AsyncSocket::post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset)
        {

        }

        void AsyncSocket::destroy(IObject * object)
        {

        }
    }
}
