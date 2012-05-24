#include "Common.h"

#include "Yeti.h"

USINGNAMESPACE2;

void test_hash_data()
{
    int hashdata1           = 0xabcd1234;
    unsigned int hashdata2  = 0xabcd1234;
    printf("Hash<int>(0xabcd1234)             = 0x%x\n", Hash<int>()(hashdata1));
    printf("Hash<unsigned int>(0xabcd1234)    = 0x%x\n", Hash<unsigned int>()(hashdata2));
}

void test_hash_str()
{
    char * hashstr1 = "12345678";
    const char * hashstr2 = "12345678";
    printf("Hash<char *>(\"12345678\")          = 0x%llx\n", Hash<char *>()(hashstr1));
    printf("Hash<const char *>(\"12345678\")    = 0x%llx\n",Hash<const char *>()(hashstr2));
}

int hash_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_hash_data();
    test_hash_str();

    return 0;
}

static TestRegister test("hash_test", hash_test);
