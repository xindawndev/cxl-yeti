#include "Common.h"

#include "Yeti.h"

USINGNAMESPACE2;

int base64_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    return 0;
}

static TestRegister test("base64_test", base64_test);
