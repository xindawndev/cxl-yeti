// CommonModule.h

#ifndef _PPBOX_COMMON_COMMON_MODULE_H_
#define _PPBOX_COMMON_COMMON_MODULE_H_

#include <daemon/Module.h>
#include <daemon/Daemon.h>

namespace ppbox
{
    namespace common
    {

        class CommonModule
            : public util::daemon::ModuleBase<CommonModule>
        {
        public:
            CommonModule(
                util::daemon::Daemon & daemon);

            CommonModule(
                util::daemon::Daemon & daemon, 
                std::string const & name);

            ~CommonModule();

        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        private:
        };

    } // namespace common
} // namespace ppbox

#endif // _PPBOX_COMMON_COMMON_MODULE_H_
