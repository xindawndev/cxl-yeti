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
    CHECK(a_list.get_item_count() == 3);
    CHECK(a_list.contains(A(2, 3)));
    CHECK(!a_list.contains(A(7, 8)));

    A a;
    CHECK(YETI_SUCCEEDED(a_list.pop_head(a)));
    CHECK(a == A(1, 2));
    CHECK(a_list.get_item_count() == 2);
    CHECK(YETI_SUCCEEDED(a_list.get(0, a)));
    CHECK(a == A(2, 3));

    A * pa = NULL;
    CHECK(YETI_SUCCEEDED(a_list.get(0, pa)));
    CHECK(pa != NULL);
    CHECK(*pa == A(2, 3));
    CHECK(a_list.get_item(1) == ++a_list.get_first_item());

    a_list.clear();
    CHECK(a_list.get_item_count() == 0);
    a_list.insert(a_list.get_first_item(), A(7, 9));
    CHECK(a_list.get_item_count() == 1);
    CHECK(*a_list.get_first_item() == A(7, 9));

    a_list.add(A(1, 2));
    CHECK(a_list.get_item_count() == 2);
    //CHECK(g_count == 3);
    CHECK(*a_list.get_first_item() == A(7, 9));
    CHECK(*a_list.get_last_item() == A(1, 2));

    a_list.insert(a_list.get_last_item(), A(3, 4));
    CHECK(a_list.get_item_count() == 3);
    CHECK(*a_list.get_last_item() == A(1, 2));

    g_apply_counter = 0;
    bool applied;
    YETI_Result res = a_list.apply_until(Test1(), UnitlResultEquals(YETI_ERROR_OUT_OF_MEMORY), &applied);
    CHECK(applied == true);
    CHECK(res == YETI_SUCCESS);
    CHECK(g_apply_counter == 2);

    g_apply_counter = 0;
    res = a_list.apply_until(Test1(), UnitlResultEquals(YETI_FAILURE), &applied);
    CHECK(applied == false);
    CHECK(res == YETI_SUCCESS);
    CHECK(g_apply_counter == 3);

    a_list.insert(List<A>::iterator(NULL), A(3, 4));
    CHECK(a_list.get_item_count() == 4);
    CHECK(*a_list.get_last_item() == A(3, 4));

    a_list.insert(a_list.get_first_item(), A(7, 8));
    CHECK(a_list.get_item_count() == 5);
    CHECK(*a_list.get_first_item() == A(7, 8));

    a_list.insert(a_list.get_item(2), A(9, 10));
    CHECK(a_list.get_item_count() == 6);
    CHECK(*a_list.get_item(2) == A(9, 10));

    a_list.erase(a_list.get_item(1));
    CHECK(a_list.get_item_count() == 5);
    CHECK(*a_list.get_item(1) == A(9, 10));
    //CHECK(g_count == 1 + a_list.get_item_count());

    List<int> i1_list;
    List<int> i2_list;
    CHECK(i1_list == i2_list);
    i1_list.add(3);
    CHECK(i1_list != i2_list);
    CHECK(!(i1_list == i2_list));
    i2_list.add(3);
    CHECK(i1_list == i2_list);
    i2_list.add(4);
    CHECK(i1_list != i2_list);
    i1_list.add(4);
    i1_list.add(5);
    i2_list.add(6);
    CHECK(i1_list != i2_list);

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
