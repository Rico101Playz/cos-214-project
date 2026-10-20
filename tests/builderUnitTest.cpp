#include <iostream>
#include <string>

#include "test_utils.h"

#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "core/WorkflowDefinition.h"

// ============================================================================
// SIMPLE TEST RUNNER
// ============================================================================

// ============================================================================
// TEST: Builder Mechanics
// ============================================================================
void test_ConcreteDefinitionBuilder() {
    std::cout << "\n--- Testing ConcreteDefinitionBuilder ---\n";
    
    ConcreteDefinitionBuilder builder;
    
    builder.reset("Test Workflow");
    builder.addStage("Stage A");
    builder.addStage("Stage B");
    builder.addWorkItem(ItemTemplate("Item 1"));
    builder.addDependency("Stage A", "Stage B");
    
    WorkflowDefinition def = builder.build();
    
    check(def.name == "Test Workflow", "Builder: resets name correctly");
    check(def.stages.size() == 2, "Builder: adds stages correctly");
    check(def.stages[0] == "Stage A", "Builder: stores stage names correctly");
    check(def.items.size() == 1, "Builder: adds work items correctly");
    check(def.dependencies.size() == 1, "Builder: adds dependencies correctly");
}

// ============================================================================
// TEST: Director & Client Mechanics
// ============================================================================
void test_FeatureDeliveryProcess() {
    std::cout << "\n--- Testing FeatureDeliveryProcess & Director ---\n";
    
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    
    process.construct(&builder);
    
    WorkflowDefinition def = builder.build();
    
    check(def.name == "Workflow based on: Feature Delivery Outline", 
          "Director: sets the correct name based on outline");
          
    check(def.stages.size() == 3, 
          "Director: adds the 3 predefined stages");
          
    check(def.stages[0] == "Initialization" && def.stages[2] == "Completion", 
          "Director: uses the correct stage names");
          
    check(def.items.size() == 2, 
          "Director: adds the 2 predefined work items");
          
    check(def.dependencies.size() == 2, 
          "Director: establishes the correct dependencies between stages");
}

// ============================================================================
// MAIN
// ============================================================================
void run_builder_tests() {
    std::cout << "\n--- Running Builder Tests ---\n";
    
    test_ConcreteDefinitionBuilder();
    test_FeatureDeliveryProcess();
    
    std::cout << "Builder Tests: " << testsPassed << " passed, " 
              << testsFailed << " failed.\n";
}