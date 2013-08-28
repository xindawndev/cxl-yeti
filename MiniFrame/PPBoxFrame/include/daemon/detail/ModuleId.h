// ModuleId.h

#ifndef _BASE_DAEMON_DETAIL_MODULE_ID_H_
#define _BASE_DAEMON_DETAIL_MODULE_ID_H_

#include "daemon/Daemon.h"

namespace base
{
    namespace daemon
    {
        namespace detail
        {

            class Id
            {
            public:
                /// Constructor.
                Id() {}
            };

            template <typename Type>
            class ModuleId
                : public Id
            {
            };

        } // namespace detail
    } // namespace daemon
} // namespace base

#endif // _BASE_DAEMON_DETAIL_MODULE_ID_H_
