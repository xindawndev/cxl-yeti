#include "Common.h"

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
int test_array()
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

    a_array.erase(1, 3);
    CHECK(a_array.get_item_count() == 3);
    CHECK(g_count == 8);
    CHECK(a_array[2] == A(9, 10));

    a_array.insert(a_array.get_first_item(), A(34, 0), 4);
    CHECK(a_array.get_item_count() == 7);
    CHECK(g_count == 12);
    CHECK(a_array[6] == A(9, 10));

    a_array.insert(a_array.get_item(5), A(116, 'e'), 200);
    CHECK(a_array.get_item_count() == 207);
    CHECK(a_array[206] == A(9, 10));

    a_array.clear();
    a_array.insert(a_array.get_first_item(), A(1, 'c'));
    CHECK(a_array.get_item_count() == 1);
    CHECK(a_array[0] == A(1, 'c'));

    a_array.insert(a_array.get_item(1), A(2, 'd'));
    CHECK(a_array.get_item_count() == 2);
    CHECK(a_array[0] == A(1, 'c'));
    CHECK(a_array[1] == A(2, 'd'));

    Array<int> * int_array = new Array<int>(100);
    CHECK(int_array->get_item_count() == 0);
    int_array->add(1);
    int_array->add(2);
    CHECK(int_array->get_item_count() == 2);
    CHECK((*int_array)[0] == 1);
    CHECK((*int_array)[1] == 2);
    int_array->clear();
    CHECK(int_array->get_item_count() == 0);
    delete int_array;

    Array<A *> c_carray;
    A * o = new A(3, 2);
    c_carray.add(o);
    CHECK(c_carray.get_item_count() == 1);
    for (int i = 0; i < 4; ++i) {
        c_carray.insert(0, new A(55, 'a'));
    }

    CHECK(c_carray.contains(o));
    A * a66 = new A(66, 'b');
    CHECK(!c_carray.contains(a66));
    delete a66;

    A ** ai = c_carray.find(ObjectComparator<A *>(o));
    CHECK(ai);
    CHECK(**ai == *o);
    c_carray.erase(ai);
    delete o;
    CHECK(c_carray.get_item_count() == 4);

    c_carray.apply(ObjectDeleter<A>());

    Array<int> i_array;
    CHECK(YETI_SUCCEEDED(i_array.resize(4, 0)));
    CHECK(i_array.get_item_count() == 4);
    i_array[0] = 3;
    i_array[1] = 7;
    i_array[2] = 9;
    i_array[3] = 12;

    Array<int> j_array = i_array;
    CHECK(i_array == j_array);
    i_array[2] = 7;
    CHECK(i_array != j_array);
    CHECK(!(i_array == j_array));
    i_array[2] = 9;
    CHECK(i_array == j_array);
    j_array.add(12);
    CHECK(i_array != j_array);
    CHECK(!(i_array == j_array));

    Array<int> k_array;
    k_array = i_array;
    CHECK(k_array == i_array);

    return 0;
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
