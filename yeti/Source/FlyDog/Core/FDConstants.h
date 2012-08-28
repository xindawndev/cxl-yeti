#ifndef _FD_CONSTANTS_H_
#define _FD_CONSTANTS_H_

#include "FlyDog.h"

namespace flydog
{
    class Constants
    {
    public:
        static Constants & get_instance();

        Constants();
        ~Constants() {}

        void set_default_device_lease(TimeInterval lease) { m_default_device_lease_ = new TimeInterval(lease); }
        Reference<TimeInterval>  get_default_device_lease() { return m_default_device_lease_; }

        void set_default_subscribe_lease(TimeInterval lease) { m_default_subscribe_lease_ = new TimeInterval(lease); }
        Reference<TimeInterval> get_default_subscribe_lease() { return m_default_subscribe_lease_; }

        void set_default_user_agent(const char * agent) { m_default_user_agent_ = new String(agent); }
        Reference<String> get_default_user_agent() { return m_default_user_agent_; }

    private:
        Reference<TimeInterval> m_default_device_lease_;
        Reference<TimeInterval> m_default_subscribe_lease_;
        Reference<String>       m_default_user_agent_;
    };
}

#endif // _FD_CONSTANTS_H_

