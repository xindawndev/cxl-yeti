// CommonModule.h

#ifndef _MINIFRAME_COMMON_COMMON_MODULE_H_
#define _MINIFRAME_COMMON_COMMON_MODULE_H_

#include <daemon/Module.h>
#include <daemon/Daemon.h>

namespace miniframe
{
    namespace common
    {

        class CommonModule
            : public base::daemon::ModuleBase<CommonModule>
        {
        public:
            CommonModule(
                base::daemon::Daemon & daemon);

            CommonModule(
                base::daemon::Daemon & daemon, 
                std::string const & name);

            ~CommonModule();

        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        private:
        };

    } // namespace common
} // namespace miniframe

#endif // _MINIFRAME_COMMON_COMMON_MODULE_H_
