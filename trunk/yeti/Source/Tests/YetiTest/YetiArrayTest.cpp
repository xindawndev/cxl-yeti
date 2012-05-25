#include "Common.h"

#include "Yeti.h"

USINGNAMESPACE2;

void test_array()
{
    Array<int> aint;
}

int array_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_array();

    return 0;
}

static TestRegister test("array_test", array_test);
