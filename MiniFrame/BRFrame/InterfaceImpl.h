#ifndef __INTERFACEIMPL_H__
#define __INTERFACEIMPL_H__

#include "Common.h"

class CDummyService;

class CInterfaceImpl
    : public boost::enable_shared_from_this<CInterfaceImpl>
{
public:
    CInterfaceImpl();
    virtual ~CInterfaceImpl();

    bool Start(boost::system::error_code & ec);
    bool Stop(boost::system::error_code & ec);
    void Reset();

    virtual void async_call_this_method_run_in_ioservice_thead(
        int param, async_callback_func_type callback);

private:
    void handle_async_call_this_method_run_in_ioservice_thead(
        boost::system::error_code const & ec, int ret, async_callback_func_type svr_callback);

    inline bool is_stopped() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_isStopped;
    }

private:
    boost::shared_ptr<CDummyService> m_dummy_service;
    mutable boost::mutex m_mutex;
    volatile bool m_isStopped;

};

#endif // __INTERFACEIMPL_H__
