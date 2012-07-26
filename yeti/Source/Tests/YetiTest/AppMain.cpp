#include "Common.h"

int main(int argc, char ** argv)
{
#if defined(WIN32) && defined(_DEBUG)
    int flags = _crtDbgFlag       | 
        _CRTDBG_ALLOC_MEM_DF      |
        _CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF;

    _CrtSetDbgFlag(flags);
    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);
#endif 
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    if (argc == 1 || std::string(argv[1]) == "all")
        start_test_all(args);
    else
        start_test(argv[1], args);

    stop_test();

    std::cout << "press any key to continue..." << std::flush;
    std::cin.get();
    return 0;
}