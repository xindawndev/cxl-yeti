#include "Common.h"

#include "Yeti.h"

USINGNAMESPACE2;

static unsigned int g_count = 0;

class A {
public:
    A() : a_(0), b_(0), c_(&a_) {
        printf("A::A()\n");
        g_count++;
    }

    A(int a, char b) : a_(a), b_(b), c_(&a_) {
        printf("A::A(%d, %d)\n", a, b);
        g_count++;
    }

    A(const A & other) : a_(other.a_), b_(other.b_), c_(&a_) {
        printf("A::A(copy: a=%d, b=%d)\n", a_, b_);
        g_count++;
    }

    ~A() {
        printf("A::~A(), a=%d, b=%d\n", a_, b_);
        g_count--;
    }

    bool check() { return c_ == &a_;}

    bool operator==(const A & other) const {
        return a_ == other.a_ && b_ == other.b_;
    }

    int a_;
    char b_;
    int * c_;
};

static int g_apply_counter = 0;

class Test1 {
public:
    YETI_Result operator()(const A & a) const {
        g_apply_counter++;
        A aa(3, 4);
        if (a == aa) return YETI_ERROR_OUT_OF_MEMORY;
        return YETI_SUCCESS;
    }
};

static int sort_test()
{
    return 0;
}

static int compare(YETI_UInt32 a, YETI_UInt32 b)
{
    return (a == b) ? 0 :((a < b) ? -1 : 1);
}

int test_list()
{
    if (sort_test()) return 1;

    List<A> a_list;
    a_list.add(A(1, 2));
    a_list.add(A(2, 3));
    a_list.add(A(3, 4));

    return 0;
}

int list_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_list();

    return 0;
}

static TestRegister test("list_test", list_test);
