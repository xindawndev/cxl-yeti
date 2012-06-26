#include <iostream>
#include "basemodule/CommonModuleBase.h"

class WorkerModule
    : public ppbox::common::CommonModuleBase<WorkerModule>
{
public:
    WorkerModule(
        util::daemon::Daemon & daemon)
        : ppbox::common::CommonModuleBase<WorkerModule>(daemon, "WorkerModule")
    {
    }

    virtual ~WorkerModule() {}

public:
    virtual boost::system::error_code startup()
    {
        boost::system::error_code ec;
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
        return ec;
    }

    virtual void shutdown()
    {
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
    }

private:
};

class TestModule
    : public ppbox::common::CommonModuleBase<TestModule>
{
public:
    TestModule(
        util::daemon::Daemon & daemon)
        : ppbox::common::CommonModuleBase<TestModule>(daemon, "TestModule")
    {
    }

    virtual ~TestModule() {}

public:
    virtual boost::system::error_code startup()
    {
        boost::system::error_code ec;
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
        return ec;
    }

    virtual void shutdown()
    {
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
    }

private:
};

int main( int argc, char **argv )
{
    util::daemon::Daemon my_daemon( "testdaemon.conf" );

    util::daemon::use_module<WorkerModule>(my_daemon);
    util::daemon::use_module<TestModule>(my_daemon);

    my_daemon.start( 5 );
    return 0;
}