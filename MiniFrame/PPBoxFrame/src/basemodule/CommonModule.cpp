// CommonModule.cpp

#include "basemodule/CommonModule.h"

namespace miniframe
{
    namespace common
    {

        CommonModule::CommonModule(
            base::daemon::Daemon & daemon)
            : base::daemon::ModuleBase<CommonModule>(daemon, "CommonModule")
        {
        }

        CommonModule::CommonModule(
            base::daemon::Daemon & daemon, 
            std::string const & name)
            : base::daemon::ModuleBase<CommonModule>(daemon, name)
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
} // namespace miniframe
