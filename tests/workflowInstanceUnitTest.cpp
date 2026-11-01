#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string>


#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "core/WorkItem.h"
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"

// Takes an item from Available all the way to Completed.
static void finish(WorkItem* item) {
    item->assign("Alice");
    item->start();
    item->complete();
    item->approve();
}

static WorkflowDefinition featureDeliveryDefinition() {
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    process.construct(&builder);
    return builder.build();
}

// ============================================================================
// TEST: building the work tree from a definition
// ============================================================================
TEST(WorkflowInstance, Tree) {

    WorkflowDefinition definition = featureDeliveryDefinition();
    WorkflowInstance instance("instance-1", definition);

    EXPECT_EQ(instance.id(), "instance-1") << "WorkflowInstance: stores its id";
    EXPECT_EQ(instance.title(), definition.name) << "WorkflowInstance: its title is the definition name";
    EXPECT_EQ(instance.state(), WorkflowInstance::CREATED) << "WorkflowInstance: starts Created";
    EXPECT_EQ(instance.currentStage(), nullptr) << "WorkflowInstance: no stage is running before start()";

    EXPECT_EQ(instance.root().childCount(), definition.stages.size()) << "WorkflowInstance: one stage node per definition stage";
    EXPECT_EQ(instance.find("Initialization"), instance.root().child(0)) << "WorkflowInstance: stages keep the definition order";
    EXPECT_TRUE(instance.item("Task A") != nullptr && instance.item("Task B") != nullptr) << "WorkflowInstance: one work item per item template";
    EXPECT_TRUE(instance.item("Task A")->parent() == instance.root().child(0)) << "WorkflowInstance: items are placed in the first stage";
    EXPECT_EQ(instance.item("Initialization"), nullptr) << "WorkflowInstance: item() ignores stages";
    EXPECT_EQ(instance.item("Nope"), nullptr) << "WorkflowInstance: item() returns nullptr for an unknown id";

    definition.stages.push_back("Extra");
    EXPECT_EQ(instance.definition().stages.size(), definition.stages.size() - 1) << "WorkflowInstance: keeps its own copy of the definition version it started from";
}

// ============================================================================
// TEST: running stages in order
// ============================================================================
TEST(WorkflowInstance, Run) {

    WorkflowDefinition definition;
    definition.name = "Feature Delivery";
    definition.stages.push_back("Build");
    definition.stages.push_back("Release");
    definition.items.push_back("Implement");
    definition.items.push_back("Test");
    WorkflowInstance instance("instance-1", definition);

    // Put a second item in Release so that stage has work of its own.
    CompositeStage* release = static_cast<CompositeStage*>(instance.find("Release"));
    release->add(std::unique_ptr<WorkNode>(new WorkItem("Deploy", "Deploy", "ReleaseManager")));

    instance.start();
    EXPECT_EQ(instance.state(), WorkflowInstance::RUNNING) << "WorkflowInstance: start() makes it Running";
    EXPECT_EQ(instance.currentStage(), instance.find("Build")) << "WorkflowInstance: the first stage runs first";
    EXPECT_TRUE(instance.item("Implement")->isReady() && instance.item("Test")->isReady()) << "WorkflowInstance: start() releases the first stage's items";
    EXPECT_TRUE(instance.item("Deploy")->status() == WorkItem::CREATED) << "WorkflowInstance: later stages are held back";

    bool threw = false;
    try {
        instance.start();
    } catch (const std::logic_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw) << "WorkflowInstance: start() twice is rejected";

    finish(instance.item("Implement"));
    instance.advance();
    EXPECT_EQ(instance.currentStage(), instance.find("Build")) << "WorkflowInstance: advance() waits while the stage has unfinished work";

    finish(instance.item("Test"));
    instance.advance();
    EXPECT_EQ(instance.currentStage(), release) << "WorkflowInstance: advance() moves to the next stage";
    EXPECT_TRUE(instance.item("Deploy")->isReady()) << "WorkflowInstance: advance() releases the next stage";

    finish(instance.item("Deploy"));
    instance.advance();
    EXPECT_EQ(instance.state(), WorkflowInstance::COMPLETED) << "WorkflowInstance: finishing the last stage completes the instance";
    EXPECT_EQ(instance.currentStage(), nullptr) << "WorkflowInstance: no stage runs once completed";

    threw = false;
    try {
        instance.cancel("Too late");
    } catch (const std::logic_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw) << "WorkflowInstance: a completed instance cannot be cancelled";
}

// ============================================================================
// TEST: edge cases and cancellation
// ============================================================================
TEST(WorkflowInstance, Edges) {

    WorkflowDefinition empty;
    empty.name = "Nothing to do";
    WorkflowInstance nothing("instance-1", empty);
    nothing.start();
    EXPECT_EQ(nothing.state(), WorkflowInstance::COMPLETED) << "WorkflowInstance: a definition with no work completes on start()";

    WorkflowDefinition flat;
    flat.name = "Checklist";
    flat.items.push_back("Only task");
    WorkflowInstance checklist("instance-2", flat);
    EXPECT_TRUE(checklist.item("Only task")->parent() == &checklist.root()) << "WorkflowInstance: without stages, items sit under the root";
    checklist.start();
    EXPECT_EQ(checklist.currentStage(), &checklist.root()) << "WorkflowInstance: without stages, the root is the running stage";
    finish(checklist.item("Only task"));
    checklist.advance();
    EXPECT_EQ(checklist.state(), WorkflowInstance::COMPLETED) << "WorkflowInstance: a stageless instance completes when its items do";

    WorkflowInstance cancelled("instance-3", featureDeliveryDefinition());
    cancelled.start();
    cancelled.item("Task A")->assign("Alice");
    cancelled.cancel("Feature dropped");
    EXPECT_EQ(cancelled.state(), WorkflowInstance::CANCELLED) << "WorkflowInstance: cancel() makes it Cancelled";
    EXPECT_EQ(cancelled.reason(), "Feature dropped") << "WorkflowInstance: cancel() records the reason";
    EXPECT_TRUE(cancelled.item("Task A")->status() == WorkItem::CANCELLED &&
          cancelled.item("Task B")->status() == WorkItem::CANCELLED) << "WorkflowInstance: cancel() cancels all unfinished work";
    cancelled.advance();
    EXPECT_EQ(cancelled.state(), WorkflowInstance::CANCELLED) << "WorkflowInstance: advance() does nothing once cancelled";
}

// ============================================================================
// MAIN
// ============================================================================

