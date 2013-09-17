#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "Common.h"

class CInterfaceImpl;

class CInterface
{
public:
    CInterface();
    virtual ~CInterface();

    virtual bool __stdcall init();
    virtual bool __stdcall uninit();
    virtual void __stdcall reset();
    virtual void __stdcall async_call_this_method_run_in_ioservice_thead(
        int param, async_callback_func_type callback);
private:
    boost::shared_ptr<CInterfaceImpl> m_interface_impl;
};

#endif // __INTERFACE_H__
