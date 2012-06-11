#include "Common.h"

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

int test_map()
{
    Map<int, int> i_map;
    int *o;
    i_map.put(1, 2);
    CHECK(i_map.get_entry_count() == 1);
    i_map.get(1,o);
    CHECK(*o == 2);
    return 0;
}

int map_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_map();

    return 0;
}

static TestRegister test("map_test", map_test);
