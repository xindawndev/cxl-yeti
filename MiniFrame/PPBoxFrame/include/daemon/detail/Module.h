// Module.h

#ifndef _BASE_DAEMON_DETAIL_MODULE_H_
#define _BASE_DAEMON_DETAIL_MODULE_H_

#include "daemon/Daemon.h"
#include "daemon/detail/ModuleId.h"

namespace base
{
    namespace daemon
    {
        namespace detail
        {

            class Module
            {
            protected:
                Module(
                    Daemon & daemon, 
                    std::string const & name = "")
                    : daemon_(daemon)
                    , name_(name)
                    , id_(NULL)
                    , next_(NULL)
                    , prev_(NULL)
                {
                }

                virtual ~Module()
                {
                }

            public:
                Daemon & get_daemon()
                {
                    return daemon_;
                }

                boost::asio::io_service & io_svc()
                {
                    return daemon_.io_svc();
                }

                std::string const & name() const
                {
                    return name_;
                }

            private:
                virtual boost::system::error_code startup() = 0;

                virtual void shutdown() = 0;

                friend class detail::ModuleRegistry;

                base::daemon::Daemon & daemon_;
                std::string const name_;
                base::daemon::detail::Id const * id_;
                base::daemon::detail::Module * next_;
                base::daemon::detail::Module * prev_;
            };

        } // namespace detail
    } // namespace daemon
} // namespace base

#endif // _BASE_DAEMON_MODULE_H_
