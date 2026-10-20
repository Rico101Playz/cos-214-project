#include <iostream>
#include "test_utils.h"

void run_command_tests();
void run_builder_tests();
void run_memento_test();
void run_strategy_tests();

int main() {
    std::cout << "========================================\n";
    std::cout << "       Running TIVIDY Test Suite\n";
    std::cout << "========================================\n";

    run_command_tests();
    run_builder_tests();
    run_memento_test();
    run_strategy_tests();
    
    std::cout << "\n========================================\n";
    std::cout << "TOTAL Results: " << testsPassed << " passed, " 
              << testsFailed << " failed.\n";
    std::cout << "========================================\n";
    
    return (testsFailed == 0) ? 0 : 1;
}