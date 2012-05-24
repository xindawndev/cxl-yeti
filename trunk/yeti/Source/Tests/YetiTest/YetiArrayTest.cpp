#include "Common.h"

#include "Yeti.h"

NAMEBEG
class A
{
};
NAMEEND

int array_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    return 0;
}

static TestRegister test("array_test", array_test);
