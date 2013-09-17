#include "DummyService.h"

CDummyService::CDummyService()
: CAsyncTask(SINGLE_THREAD_POOL_SIZE)
{
}

CDummyService::~CDummyService()
{
}

bool CDummyService::Start(boost::system::error_code & ec)
{
    ec.clear();
    CAsyncTask::Start();
    return true;
}

bool CDummyService::Stop(boost::system::error_code & ec)
{
    ec.clear();
    CAsyncTask::Stop();
    return true;
}

void CDummyService::this_method_run_in_ioservice_thead(
                                        int param,
                                        service_callback_func_type callback)
{
    boost::system::error_code ec;
    int sum = 0;
    for (int i = 0; i < param; ++i) {
        // do something
        sum += i + 1;
    }
    callback(ec, sum, placeholder_func);
}