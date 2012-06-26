// CommonModuleBase.h

#ifndef _PPBOX_COMMON_COMMON_MODULE_BASE_H_
#define _PPBOX_COMMON_COMMON_MODULE_BASE_H_

#include "basemodule/CommonModule.h"

namespace ppbox
{
    namespace common
    {

        template <
            typename ModuleType
        >
        class CommonModuleBase
            : public util::daemon::ModuleBase<CommonModuleBase<ModuleType> >
        {
        public:
            CommonModuleBase(
                util::daemon::Daemon & daemon, 
                std::string const & name = "")
                : util::daemon::ModuleBase<CommonModuleBase<ModuleType> >(daemon, name)
                , common_(util::daemon::use_module<CommonModule>(daemon))
            {
            }

        public:
            ppbox::common::CommonModule & common()
            {
                return common_;
            }

        private:
            ppbox::common::CommonModule & common_;
        };

    } // namespace common
} // namespace ppbox

#endif // _PPBOX_COMMON_COMMON_MODULE_H_
