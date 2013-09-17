#include "InterfaceImpl.h"
#include "DummyService.h"
#include "Errors.h"

CInterfaceImpl::CInterfaceImpl()
: m_dummy_service(new CDummyService())
, m_isStopped(true)
{
}

CInterfaceImpl::~CInterfaceImpl()
{
}

bool CInterfaceImpl::Start(boost::system::error_code & ec)
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (!m_isStopped) {
            ec = interface_error::already_started;
            return false;
        }
    }
    bool ret = m_dummy_service->Start(ec);
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_isStopped = false;
    }
    return ret;
}

bool CInterfaceImpl::Stop(boost::system::error_code & ec)
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_isStopped = true;
    }
    bool ret = false;
    if (m_dummy_service.get()) {
        ret = m_dummy_service->Stop(ec);
    }
    return ret;
}

void CInterfaceImpl::Reset()
{
    boost::system::error_code ec;
    Stop(ec);
    Start(ec);
}

void CInterfaceImpl::async_call_this_method_run_in_ioservice_thead(
    int param, async_callback_func_type callback)
{
    if (is_stopped())
        return;
    service_callback_func_type service_cb(
        boost::bind(
        &CInterfaceImpl::handle_async_call_this_method_run_in_ioservice_thead,
        shared_from_this(), _1, _2, callback));
    m_dummy_service->Post(
        boost::bind(
        &CDummyService::this_method_run_in_ioservice_thead, m_dummy_service,
        param, service_cb));
}

void CInterfaceImpl::handle_async_call_this_method_run_in_ioservice_thead(
    boost::system::error_code const & ec, int ret, async_callback_func_type svr_callback)
{
    svr_callback(ec, ret);
}