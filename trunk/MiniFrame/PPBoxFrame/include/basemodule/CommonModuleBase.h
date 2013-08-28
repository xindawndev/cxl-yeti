// CommonModuleBase.h

#ifndef _MINIFRAME_COMMON_COMMON_MODULE_BASE_H_
#define _MINIFRAME_COMMON_COMMON_MODULE_BASE_H_

#include "basemodule/CommonModule.h"

namespace miniframe
{
    namespace common
    {

        template <
            typename ModuleType
        >
        class CommonModuleBase
            : public base::daemon::ModuleBase<CommonModuleBase<ModuleType> >
        {
        public:
            CommonModuleBase(
                base::daemon::Daemon & daemon, 
                std::string const & name = "")
                : base::daemon::ModuleBase<CommonModuleBase<ModuleType> >(daemon, name)
                , common_(base::daemon::use_module<CommonModule>(daemon))
            {
            }

        public:
            miniframe::common::CommonModule & common()
            {
                return common_;
            }

        private:
            miniframe::common::CommonModule & common_;
        };

    } // namespace common
} // namespace miniframe

#endif // _MINIFRAME_COMMON_COMMON_MODULE_H_
