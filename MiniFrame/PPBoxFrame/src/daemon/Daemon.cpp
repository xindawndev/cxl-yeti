// Daemon.cpp

#include "daemon/Daemon.h"

#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace util
{
    namespace daemon
    {

        Daemon::Daemon()
            : io_work_(NULL)
            , module_registry_(new detail::ModuleRegistry(*this))
        {
            //logger_.load_config(config_);
        }

        Daemon::Daemon(
            std::string const & conf)
            : io_work_(NULL)
            , module_registry_(new detail::ModuleRegistry(*this))
        {
            //logger_.load_config(config_);
        }

        Daemon::~Daemon()
        {
            quick_stop();
            delete module_registry_;
        }

        int Daemon::parse_cmdline(
            int argc, 
            char const * argv[])
        {
            return 0;
        }

        static void startup_notify(
            boost::system::error_code & result2, 
            boost::mutex & mutex, 
            boost::condition_variable & cond, 
            boost::system::error_code const & result)
        {
            boost::mutex::scoped_lock lock(mutex);
            result2 = result;
            cond.notify_all();
        }

        boost::system::error_code Daemon::start(
            size_t concurrency)
        {
            io_work_ = new boost::asio::io_service::work(io_svc_);
            boost::system::error_code result;
            fprintf(stdout, "[start] beg");
            if (concurrency == 0) {
                result = module_registry_->startup();
            } else {
                boost::mutex mutex;
                boost::condition_variable cond;
                boost::mutex::scoped_lock lock(mutex);
                io_svc_.post(boost::bind(startup_notify, 
                    boost::ref(result), 
                    boost::ref(mutex), 
                    boost::ref(cond), 
                    boost::bind(&detail::ModuleRegistry::startup, module_registry_)));
                for (size_t i = 0; i < concurrency; ++i) {
                    th_grp_.create_thread(boost::bind(&boost::asio::io_service::run, &io_svc_));
                }
                cond.wait(lock);
            }
            fprintf(stdout, "[start] end");
            if (result) {
                fprintf(stdout, "[stop] beg");
                delete io_work_;
                io_work_ = NULL;
                run();
            }
            return result;
        }

        boost::system::error_code Daemon::start(
            start_call_back_type const & start_call_back)
        {
            fprintf(stdout, "[start] beg");
            io_work_ = new boost::asio::io_service::work(io_svc_);
            boost::system::error_code result;
            result = module_registry_->startup();
            fprintf(stdout, "[start] end");
            start_call_back(result);
            if (result) {
            	fprintf(stdout, "[stop] beg");
                delete io_work_;
                io_work_ = NULL;
            }
            run();
            return result;
        }

        void Daemon::run()
        {
            if (th_grp_.size()) {
                th_grp_.join_all();
                io_svc_.reset();
            } else {
                io_svc_.run();
                io_svc_.reset();
            }
            fprintf(stdout, "[stop] end");
        }

        void Daemon::stop(
            bool wait)
        {
            delete io_work_;
            io_work_ = NULL;
            io_svc_.post(
                boost::bind(&detail::ModuleRegistry::shutdown, module_registry_));
            if (wait) {
                fprintf(stdout, "[stop] beg");
                run();
            }
        }

        void Daemon::post_stop()
        {
            io_svc_.post(boost::bind(&Daemon::stop, this, false));
        }

        void Daemon::quick_stop()
        {
            if (io_work_) {
                delete io_work_;
                io_work_ = NULL;
            }
            io_svc_.stop();
            if (th_grp_.size()) {
                th_grp_.join_all();
                io_svc_.reset();
            }
        }

        bool Daemon::is_started() const
        {
            return module_registry_->is_started();
        }

    } // namespace daemon
} // namespace util
