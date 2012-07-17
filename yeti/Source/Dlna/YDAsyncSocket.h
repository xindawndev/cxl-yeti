#ifndef _YETI_DLNA_YDASYNCSOCKET_H_
#define _YETI_DLNA_YDASYNCSOCKET_H_

#include "YDCommon.h"
#include "YDEngine.h"

namespace yeti
{
    namespace dlna
    {

        class AsyncSocket : public IObject
        {
        public:
            AsyncSocket(const Engine & engine);
            virtual ~AsyncSocket() {}

        public:
            const Engine & engine() const {
                return m_engine_;
            }

        public:
            virtual void pre_process(IObject * object, void * readset, void * writeset, void * errorset, int * blocktime);
            virtual void post_process(IObject * object, int slct, void * readset, void * writeset, void * errorset);
            virtual void destroy(IObject * object);

        private:
            const Engine & m_engine_;
        };

    }
}

#endif // _YETI_DLNA_YDASYNCSOCKET_H_
