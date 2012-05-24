#include "Common.h"

#include "Yeti.h"

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

    iterator find(element elm, YETI_Ordinal n = 0) {
        int i;
        if (n < 0 || n >= ARRAY_SIZE) return NULL;
        for (i = n; i < ARRAY_SIZE; ++i)
        {
            if (m_member_[i] == elm) {
                return m_member_ + i;
            }
            return NULL;
        }
    }

private:
    T m_member_[ARRAY_SIZE];
};

int common_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        std::cout << i << " : " << args[i] << std::endl;
    }

    A<int> * pa = new A<int>();

    ObjectDeleter< A< int > >()(pa);

    return 0;
}

static TestRegister test("common_test", common_test);
