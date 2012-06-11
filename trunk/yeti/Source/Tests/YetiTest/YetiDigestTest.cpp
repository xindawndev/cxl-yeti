#include "Common.h"

USINGNAMESPACE2;

int test_digest()
{
    Digest *dig = NULL;
    Digest::create(Digest::ALGORITHM_MD5, dig);

    return 0;
}

int digest_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    return 0;
}

static TestRegister test("digest_test", digest_test);
