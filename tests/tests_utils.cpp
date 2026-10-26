#include "test_utils.h"
#include <iostream>

int testsPassed = 0;
int testsFailed = 0;

void check(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        std::cerr << "[FAIL] " << testName << "\n";
        testsFailed++;
    }
}