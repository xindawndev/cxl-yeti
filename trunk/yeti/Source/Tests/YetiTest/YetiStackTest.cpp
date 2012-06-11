#include "Common.h"

USINGNAMESPACE2;

int test_stack()
{
    Stack<int> i_stack;
    int i = 0;
    CHECK(YETI_FAILED(i_stack.pop(i)));
    CHECK(YETI_FAILED(i_stack.peek(i)));
    CHECK(YETI_SUCCEEDED(i_stack.push(4)));
    CHECK(YETI_SUCCEEDED(i_stack.push(5)));
    CHECK(YETI_SUCCEEDED(i_stack.push(6)));
    CHECK(YETI_SUCCEEDED(i_stack.pop(i)));
    CHECK(i == 6);
    CHECK(YETI_SUCCEEDED(i_stack.peek(i)));
    CHECK(i == 5);
    CHECK(YETI_SUCCEEDED(i_stack.pop(i)));
    CHECK(i == 5);
    CHECK(YETI_SUCCEEDED(i_stack.pop(i)));
    CHECK(i == 4);

    return 0;
}

int stack_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_stack();

    return 0;
}

static TestRegister test("stack_test", stack_test);
