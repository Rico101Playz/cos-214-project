#include <iostream>
#include "test_utils.h"

void run_command_tests();
void run_builder_tests();
void run_memento_test();
void run_strategy_tests();
void run_composite_tests();
void run_iterator_tests();
void run_workflow_instance_tests();
void run_facade_tests();
void run_workitem_state_tests();
void run_instance_state_tests();
void run_stage_mode_tests();
void run_composite_unit_tests();
void run_state_composite_integration_tests();
void run_observer_tests();

void runChainOfResponsibilityTests();
void run_factory_tests();
int main() {
    std::cout << "========================================\n";
    std::cout << "       Running TIVIDY Test Suite\n";
    std::cout << "========================================\n";

    run_command_tests();
    run_builder_tests();
    run_memento_test();
    run_strategy_tests();
    run_composite_tests();
    run_iterator_tests();
    run_workflow_instance_tests();
    run_facade_tests();
    run_workitem_state_tests();
    run_instance_state_tests();
    run_stage_mode_tests();
    run_composite_unit_tests();
    run_state_composite_integration_tests();

    runChainOfResponsibilityTests();
    run_factory_tests();
    run_observer_tests();  
    
    std::cout << "\n========================================\n";
    std::cout << "TOTAL Results: " << testsPassed << " passed, "
              << testsFailed << " failed.\n";
    std::cout << "========================================\n";

    return (testsFailed == 0) ? 0 : 1;
}
