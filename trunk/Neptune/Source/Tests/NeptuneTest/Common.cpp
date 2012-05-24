#include "Common.h"

std::map<std::string, FUNC_TEST> * tests_ = NULL;

int reg_test(
             std::string const & name, 
             FUNC_TEST test)
{
    if (!tests_) {
        tests_ = new std::map<std::string, FUNC_TEST>;
    }
    std::pair<std::map<std::string, FUNC_TEST>::iterator, bool> ret = 
        tests_->insert(std::make_pair(name, test));
    return ret.second ? EC_SUCCESS : EC_ALREADY_EXIST;
}

int run_test(
              std::map<std::string, FUNC_TEST>::iterator i, 
              std::vector<std::string> args)
{
    if (!i->second) {
        return EC_TEST_NOT_EXIST;
    }

    std::cout << "start test [" << i->first << "]" << std::endl;
    return i->second(args);
    // i->second = NULL;
};

int start_test(
               std::string const & name, 
               std::vector<std::string> args)
{
    std::map<std::string, FUNC_TEST>::iterator i = tests_->find(name);
    if (i == tests_->end()) {
        std::cout << "test instance not found" << std::endl;
        return EC_ITEM_NOT_EXIST;
    } else {
        run_test(i, args);
        return EC_SUCCESS;
    }
}

int start_test_all(
                   std::vector<std::string> args)
{
    if (!tests_)
        return EC_SUCCESS;
    std::map<std::string, FUNC_TEST>::iterator i = tests_->begin();
    for (; i != tests_->end(); ++i) {
        run_test(i, args);
    }
    return EC_SUCCESS;
}

int stop_test()
{
    if (tests_) {
        delete tests_;
        tests_ = NULL;
    }
    return EC_SUCCESS;
}
