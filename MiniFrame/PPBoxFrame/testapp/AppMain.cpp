#include <iostream>
#include "basemodule/CommonModuleBase.h"

class WorkerModule
    : public miniframe::common::CommonModuleBase<WorkerModule>
{
public:
    WorkerModule(
        base::daemon::Daemon & daemon)
        : miniframe::common::CommonModuleBase<WorkerModule>(daemon, "WorkerModule")
    {
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
    }

    virtual ~WorkerModule()
    {
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
    }

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
    : public miniframe::common::CommonModuleBase<TestModule>
{
public:
    TestModule(
        base::daemon::Daemon & daemon)
        : miniframe::common::CommonModuleBase<TestModule>(daemon, "TestModule")
    {
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
    }

    virtual ~TestModule()
    {
        printf( "(%d): %s\n", __LINE__, __FUNCTION__ );
    }

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
    base::daemon::Daemon my_daemon( "testdaemon.conf" );

    base::daemon::use_module<WorkerModule>(my_daemon);
    base::daemon::use_module<TestModule>(my_daemon);

    my_daemon.start( 5 );
    getchar();
    my_daemon.stop(true);
    getchar();
    return 0;
}