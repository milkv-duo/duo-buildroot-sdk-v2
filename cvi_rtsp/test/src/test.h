#include <stdio.h>
#include <iostream>

#define CHECK_EQ(x, y) if (x != y) { return -1; }

struct TestCase {
    const char *name;
    int (*test)();
};

void run_all_tests(TestCase *test_cases, bool abort_if_fail)
{
    if (test_cases == NULL) {
        return;
    }

    for (int i = 0; test_cases[i].test != NULL; i++) {
        std::cout << std::endl;
        std::cout << "Testing... " << test_cases[i].name << std::endl;;
        if (0 == test_cases[i].test()) {
            std::cout << "Testing... " << test_cases[i].name << " Success" << std::endl;;
        } else {
            std::cout << "Testing... " << test_cases[i].name << " Fail" << std::endl;;
            if (abort_if_fail) {
                exit(-1);
            }
        }
    }
}
