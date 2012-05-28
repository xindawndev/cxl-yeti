#include "Common.h"

#include "Yeti.h"

USINGNAMESPACE2;

static unsigned int g_count = 0;

class A {
public:
    A() : a_(0), b_(0), c_(&a_) {
        g_count++;
    }

    A(int a, char b) : a_(a), b_(b), c_(&a_) {
        g_count++;
    }

    A(const A & other) : a_(other.a_), b_(other.b_), c_(&a_) {
        g_count++;
    }

    ~A() {
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
void test_array()
{
    YETI_Result res;
    Array<int> aint;

    aint.add(7);
    CHECK(aint[0] == 7);

    Array<A> a_array;
    a_array.add(A(1, 2));
    a_array.add(A(3, 4));
    a_array.reserve(100);
    a_array.add(A(4, 5));

    CHECK(g_count == 3);

    Array<A> b_array = a_array;
    CHECK(g_count == 6);
    CHECK(b_array.get_item_count() == a_array.get_item_count());
    CHECK(a_array == b_array);
    CHECK(a_array[0] == b_array[0]);
    b_array[0] = A(7, 8);
    CHECK(g_count == 6);
    CHECK(!(a_array[0] == b_array[0]));

    a_array.resize(2);
    CHECK(g_count == 5);
    CHECK(a_array.get_item_count() == 2);
    b_array.resize(5);
    CHECK(g_count == 7);
    CHECK(b_array[4].a_ == 0);
    CHECK(b_array[4].b_ == 0);

    a_array.resize(6, A(9, 10));
    CHECK(g_count == 11);
    CHECK(a_array.get_item_count() == 6);
    CHECK(a_array[5] == A(9, 10));

    for (YETI_Ordinal i = 0; i < a_array.get_item_count(); ++i) {
        a_array[i].check();
    }
    for (YETI_Ordinal i = 0; i < b_array.get_item_count(); ++i) {
        b_array[i].check();
    }

    res = a_array.erase(&a_array[6]);
    CHECK(res != YETI_SUCCESS);
    a_array.erase(&a_array[2]);
    CHECK(a_array.get_item_count() == 5);
    CHECK(g_count == 10);
    CHECK(a_array[4] == A(9, 10));

    a_array.insert(a_array.get_item(1), A(3, 110), 1);
    CHECK(a_array.get_item_count() == 6);
    CHECK(g_count == 11);
    CHECK(a_array[1] == A(3, 110));
    CHECK(a_array[5] == A(9, 10));
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
