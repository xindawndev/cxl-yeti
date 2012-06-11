#include "Common.h"

USINGNAMESPACE2;

int state_machine_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    return 0;
}

static TestRegister test("state_machine_test", state_machine_test);
