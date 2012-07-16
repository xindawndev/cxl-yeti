#include "Common.h"

USINGNAMESPACE2;

class T1 : public Runnable
{
    void run() {
        yeti_debug("*** T1 running **\n");
        TimeInterval duration(1.0f);
        yeti_debug("*** T1 sleeping **\n");
        System::sleep(duration);
        yeti_debug("*** T1 done ***\n");
    }
};

int test1()
{
    yeti_debug("--- test1 start ---\n");

    T1 runnable;

    yeti_debug("+++ creating non-detached thread +++\n");
    Thread * thread1 = new Thread(runnable);
    yeti_debug("+++ starting non-detached thread +++\n");
    thread1->start();
    yeti_debug("+++ waiting for non-detached thread +++\n");
    YETI_Result result = thread1->wait();
    CHECK(YETI_SUCCEEDED(result));
    yeti_debug("+++ deleting for non-detached thread +++\n");
    delete thread1;

    return YETI_SUCCESS;
}

#if defined(WIN32) && defined(_DEBUG)
static int alloc_hook( int allocType, void *userData, size_t size, int blockType, 
                     long requestNumber, const unsigned char *filename, int lineNumber)
{
    (void)allocType;
    (void)userData;
    (void)size;
    (void)blockType;
    (void)requestNumber;
    (void)lineNumber;
    (void)filename;
    return 1;
}
#endif

int thread_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

#if defined(WIN32) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF |
        _CRTDBG_LEAK_CHECK_DF);
    _CrtSetAllocHook(alloc_hook);
#endif

    test1();

    yeti_debug("- program done -\n");

    return 0;
}

static TestRegister test("thread_test", thread_test);
