#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>

#define CHECK(x) {                                  \
    if (!(x)) {                                     \
    printf("TEST FAILED line %d\n", __LINE__);      \
    return;                                         \
    }                                               \
}

enum _eErrorCode
{
    EC_SUCCESS = 0,
    EC_ALREADY_EXIST,
    EC_ITEM_NOT_EXIST,
    EC_TEST_NOT_EXIST,
};

typedef int (*FUNC_TEST)(std::vector<std::string> args);

int reg_test(
             std::string const & name, 
             FUNC_TEST test);

int start_test(
               std::string const & name, 
               std::vector<std::string> args);

int start_test_all(
                   std::vector<std::string> args);

int stop_test();

struct TestRegister
{
    TestRegister(
        std::string const & name,
        FUNC_TEST test)
    {
        total_++;
        std::pair<std::map<std::string, int>::iterator, bool> ret = \
            reg_rets_.insert(std::make_pair(name, reg_test(name, test)));
        ret.second ? succeed_++ : reg_rets_[name] = EC_ALREADY_EXIST; 
    }

    int total_;
    int succeed_;
    std::map<std::string, int> reg_rets_;
};

#endif
