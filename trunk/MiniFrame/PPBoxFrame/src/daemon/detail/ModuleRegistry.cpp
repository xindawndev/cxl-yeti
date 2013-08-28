// ModuleRegistry.cpp

#include "daemon/Daemon.h"

namespace base
{
    namespace daemon
    {
        namespace detail
        {

            ModuleRegistry::ModuleRegistry(
                Daemon & daemon)
                : daemon_(daemon)
                , first_module_(NULL)
                , last_module_(NULL)
                , is_started_(false)
            {
            }

            ModuleRegistry::~ModuleRegistry()
            {
                while (last_module_) {
                    Module * prev_module = last_module_->prev_;
                    delete last_module_;
                    last_module_ = prev_module;
                }
            }

            boost::system::error_code ModuleRegistry::startup()
            {
                boost::mutex::scoped_lock lock(mutex_);
                boost::system::error_code ec;
                Module * module = first_module_;
                while (module) {
                    fprintf(stdout, "starting module %s\n", module->name().c_str());
                    ec = module->startup();
                    if (ec) {
                        fprintf(stderr, "start module %s failed: %s\n", module->name().c_str(), ec.message().c_str());
                        break;
                    }
                    module = module->next_;
                }
                if (module) {
                    module = module->prev_;
                    while (module) {
                        fprintf(stdout, "shutdowning module %s\n", module->name().c_str());
                        module->shutdown();
                        module = module->prev_;
                    }
                } else {
                    is_started_ = true;
                }
                return ec;
            }

            void ModuleRegistry::shutdown()
            {
                boost::mutex::scoped_lock lock(mutex_);
                if (!is_started_)
                    return;
                Module * module = last_module_;
                while (module) {
                    fprintf(stdout, "shutdowning module %s\n", module->name().c_str());
                    module->shutdown();
                    module = module->prev_;
                }
                is_started_ = false;
            }

        } // namespace detail
    } // namespace daemon
} // namespace base
