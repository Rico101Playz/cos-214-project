#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>

extern int testsPassed;
extern int testsFailed;

void check(bool condition, const std::string& testName);

#endif // TEST_UTILS_H