#include <gtest/gtest.h>
#include <iostream>
#include <string>


#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "core/WorkflowDefinition.h"

// ============================================================================
// TEST: Builder Mechanics
// ============================================================================
TEST(Builder, ConcreteDefinitionBuilder) {
    
    ConcreteDefinitionBuilder builder;
    
    builder.reset("Test Workflow");
    builder.addStage("Stage A");
    builder.addStage("Stage B");
    builder.addWorkItem(ItemTemplate("Item 1"));
    builder.addDependency("Stage A", "Stage B");
    
    WorkflowDefinition def = builder.build();
    
    EXPECT_EQ(def.name, "Test Workflow") << "Builder: resets name correctly";
    EXPECT_EQ(def.stages.size(), 2u) << "Builder: adds stages correctly";
    EXPECT_EQ(def.stages[0], "Stage A") << "Builder: stores stage names correctly";
    EXPECT_EQ(def.items.size(), 1u) << "Builder: adds work items correctly";
    EXPECT_EQ(def.dependencies.size(), 1u) << "Builder: adds dependencies correctly";
}

// ============================================================================
// TEST: Director & Client Mechanics
// ============================================================================
TEST(Builder, FeatureDeliveryProcess) {
    
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    
    process.construct(&builder);
    
    WorkflowDefinition def = builder.build();
    
    EXPECT_EQ(def.name, "Workflow based on: Feature Delivery Outline") << "Director: sets the correct name based on outline";
          
    EXPECT_EQ(def.stages.size(), 3u) << "Director: adds the 3 predefined stages";
          
    EXPECT_TRUE(def.stages[0] == "Initialization" && def.stages[2] == "Completion") << "Director: uses the correct stage names";
          
    EXPECT_EQ(def.items.size(), 2u) << "Director: adds the 2 predefined work items";
          
    EXPECT_EQ(def.dependencies.size(), 2u) << "Director: establishes the correct dependencies between stages";
}

// ============================================================================
// MAIN
// ============================================================================
