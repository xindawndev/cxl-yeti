#ifndef __DUMMYSERVICE_H__
#define __DUMMYSERVICE_H__

#include "Common.h"
#include "AsyncTask.h"

class CDummyService
    : public CAsyncTask
{
public:
    CDummyService();
    virtual ~CDummyService();

    bool Start(boost::system::error_code & ec);
    bool Stop(boost::system::error_code & ec);

    void this_method_run_in_ioservice_thead(
        int param, service_callback_func_type callback);

};

#endif // __DUMMYSERVICE_H__
