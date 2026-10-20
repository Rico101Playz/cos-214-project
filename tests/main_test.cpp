#include <iostream>
#include "test_utils.h"

void run_command_tests();
void run_builder_tests();
void run_strategy_tests();
void run_composite_tests();
void run_iterator_tests();
void run_workflow_instance_tests();

int main() {
    std::cout << "========================================\n";
    std::cout << "       Running TIVIDY Test Suite\n";
    std::cout << "========================================\n";

    run_command_tests();
    run_builder_tests();
    run_strategy_tests();
    run_composite_tests();
    run_iterator_tests();
    run_workflow_instance_tests();
    
    std::cout << "\n========================================\n";
    std::cout << "TOTAL Results: " << testsPassed << " passed, " 
              << testsFailed << " failed.\n";
    std::cout << "========================================\n";
    
    return (testsFailed == 0) ? 0 : 1;
}