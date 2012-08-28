#include "FDConstants.h"
//#include "FDHttp.h"

namespace flydog
{

    static Constants constants;

    Constants::Constants()
    {
        //set_default_user_agent(FD_HTTP_DEFAULT_USER_AGENT);
        set_default_device_lease(TimeInterval(1800.));
        set_default_subscribe_lease(TimeInterval(1800.));
    }

    Constants & Constants::get_instance()
    {
        return constants;
    }

}