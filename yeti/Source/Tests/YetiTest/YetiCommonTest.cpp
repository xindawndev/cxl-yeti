#include "Common.h"

USINGNAMESPACE2;

#define ARRAY_SIZE 16

template < class T >
class A
{
public:
    typedef T element;
    typedef T * iterator;

    A() {
        int i = 0;
        for (i = 0; i < ARRAY_SIZE; ++i)
        {
            m_member_[i] = i + 1;
        }
    }

    bool operator==(const A<T> & others) const {
        if (!memcmp(others.m_member_, m_member_, ARRAY_SIZE)) {
            return true;
        }
        return false;
    }

    iterator find(element elm, YETI_Ordinal n = 0) {
        int i;
        if (n < 0 || n >= ARRAY_SIZE) return NULL;
        for (i = n; i < ARRAY_SIZE; ++i)
        {
            if (m_member_[i] == elm) {
                return m_member_ + i;
            }
        }
        return NULL;
    }

private:
    T m_member_[ARRAY_SIZE];
};

int test_ObjectDeleter()
{
    A<int> * pa = new A<int>();

    ObjectDeleter< A< int > >()(pa);
    CHECK(pa == NULL);

    return 0;
}

int test_ObjectComparator()
{
    A<int> a, b;
    ObjectComparator< A< int > > o1(a);
    CHECK(o1(b));

    return 0;
}

int test_container_find()
{
    A<int> a;
    A<int>::element elm;
    YETI_Result result = container_find(a, 5, elm, 2);
    CHECK(YETI_SUCCESS == result);
    CHECK(elm == 5);
    result = container_find(a, 5, elm, 6);
    CHECK(YETI_SUCCESS != result);

    return 0;
}

int test_container_find1()
{
    A<int> a;
    A<int>::iterator iter;
    YETI_Result result = container_find(a, 5, iter, 2);
    CHECK(YETI_SUCCESS == result);
    CHECK(*iter == 5);
    result = container_find(a, 5, iter, 6);
    CHECK(YETI_SUCCESS != result);

    return 0;
}

int common_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    test_ObjectDeleter();
    test_ObjectComparator();
    test_container_find();
    test_container_find1();

    return 0;
}

static TestRegister test("common_test", common_test);
