#include "Interface.h"

void callback_func(boost::system::error_code const & ec, int result)
{
    std::cout << "ec = " << ec.message() << " result = " << result << std::endl;
}

int main( int argc, char **argv )
{
    CInterface interface;
    interface.init();
    interface.async_call_this_method_run_in_ioservice_thead(100, callback_func);
    getchar();
    interface.uninit();
    return 0;
}
