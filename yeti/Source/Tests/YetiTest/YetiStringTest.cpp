#include "Common.h"

#include "Yeti.h"

USINGNAMESPACE2;

int test_string()
{
    printf(":: testing empty string\n");
    String s;
    printf("sizeof(s)=%d, chars = '%s'\n", (int)sizeof(s), s.get_chars());

    printf(":: testing allocation, new and delete\n");
    String * n0 = new String("Hello");
    delete n0;
    //String n1 = "Bye";
    //n1 = "ByeBye";

    return 0;
}

int string_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_string();

    return 0;
}

static TestRegister test("string_test", string_test);
