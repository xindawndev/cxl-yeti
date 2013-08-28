// Module.h

#ifndef _BASE_DAEMON_MODULE_H_
#define _BASE_DAEMON_MODULE_H_

#include "daemon/Daemon.h"
#include "daemon/detail/Module.h"

namespace base
{
    namespace daemon
    {

        template <typename Type>
        class ModuleBase
            : public detail::Module
        {
        public:
            static detail::ModuleId<Type> id;

            // Constructor.
            ModuleBase(
                Daemon & daemon, 
                std::string const & name = "")
                : detail::Module(daemon, name)
            {
            }
        };

        template <typename Type>
        detail::ModuleId<Type> ModuleBase<Type>::id;

    } // namespace daemon
} // namespace bast

#endif // _BASE_DAEMON_MODULE_H_
