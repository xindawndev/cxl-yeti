// CommonModule.cpp

#include "basemodule/CommonModule.h"

namespace ppbox
{
    namespace common
    {

        CommonModule::CommonModule(
            util::daemon::Daemon & daemon)
            : util::daemon::ModuleBase<CommonModule>(daemon, "CommonModule")
        {
        }

        CommonModule::CommonModule(
            util::daemon::Daemon & daemon, 
            std::string const & name)
            : util::daemon::ModuleBase<CommonModule>(daemon, name)
        {
        }

        CommonModule::~CommonModule()
        {
        }

        boost::system::error_code CommonModule::startup()
        {
            return boost::system::error_code();
        }

        void CommonModule::shutdown()
        {
        }

    } // namespace common
} // namespace ppbox
