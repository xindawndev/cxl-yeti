#include "Common.h"

USINGNAMESPACE2;

int test_hash_data()
{
    int hashdata1           = 0xabcd1234;
    unsigned int hashdata2  = 0xabcd1234;
    printf("Hash<int>(0xabcd1234)             = 0x%x\n", Hash<int>()(hashdata1));
    printf("Hash<unsigned int>(0xabcd1234)    = 0x%x\n", Hash<unsigned int>()(hashdata2));

    return 0;
}

int test_hash_str()
{
    char * hashstr1 = "12345678";
    const char * hashstr2 = "12345678";
    printf("Hash<char *>(\"12345678\")          = 0x%llx\n", Hash<char *>()(hashstr1));
    printf("Hash<const char *>(\"12345678\")    = 0x%llx\n",Hash<const char *>()(hashstr2));

    return 0;
}

int hash_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_hash_data();
    test_hash_str();

    YETI_UInt32 h32 = cxl::yeti::fnv1a_hash32((const YETI_UInt8*)"curds and whey", 14);
    h32 = fnv1a_hashstr32("curds and whey");
    h32 = Hash<const char*>()("curds and whey");

    YETI_UInt64 h64 = fnv1a_hash64((const YETI_UInt8*)"curds and whey", 14);
    h64 = fnv1a_hashstr64("curds and whey");

    return 0;
}

static TestRegister test("hash_test", hash_test);
