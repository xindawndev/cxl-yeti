#include "Interface.h"
#include "InterfaceImpl.h"

CInterface::CInterface()
: m_interface_impl(new CInterfaceImpl())
{

}

CInterface::~CInterface()
{

}

bool CInterface::init()
{
    boost::system::error_code ec;
    return m_interface_impl->Start(ec);
}

bool CInterface::uninit()
{
    boost::system::error_code ec;
    return m_interface_impl->Stop(ec);
}

void CInterface::reset()
{
    m_interface_impl->Reset();
}

void CInterface::async_call_this_method_run_in_ioservice_thead(
    int param, async_callback_func_type callback)
{
    m_interface_impl->async_call_this_method_run_in_ioservice_thead(param, callback);
}
