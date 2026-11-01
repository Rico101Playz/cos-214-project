#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


#include "core/Participant.h"
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "core/WorkItem.h"
#include "patterns/structural/Tividy.h"

// Runs the action and reports whether it threw the given exception type.
template <typename Exception, typename Action>
static bool throws(Action action)
{
      try
      {
            action();
      }
      catch (const Exception &)
      {
            return true;
      }
      return false;
}

// Build -> Review, with the work for Build supplied by the caller.
static WorkflowDefinition deliveryDefinition()
{
      WorkflowDefinition definition;
      definition.name = "Feature Delivery";
      definition.stages.push_back("Build");
      definition.stages.push_back("Review");
      definition.items.push_back("Implement");
      definition.items.push_back("Test");
      return definition;
}

static Tividy *makeEngine()
{
      Tividy *engine = new Tividy("Acme Software");
      engine->organisation().addParticipant("Alice", "Developer");
      engine->organisation().addParticipant("Bob", "Developer");
      engine->organisation().addParticipant("Carol", "QA");
      return engine;
}

static int loadOf(Tividy &engine, const std::string &name)
{
      return engine.organisation().find(name)->load();
}

// ============================================================================
// TEST: definitions and instances
// ============================================================================
TEST(Facade, Definitions) {

      std::unique_ptr<Tividy> engine(makeEngine());
      EXPECT_TRUE(engine->organisation().name() == "Acme Software") << "Tividy: owns the named organisation";
      EXPECT_TRUE(engine->hasDefinition("delivery") == false) << "Tividy: starts with no definitions";

      engine->loadDefinition("outline");
      EXPECT_TRUE(engine->hasDefinition("outline")) << "Tividy: loadDefinition(name) builds and stores a definition";

      engine->loadDefinition("delivery", deliveryDefinition());
      EXPECT_TRUE(engine->hasDefinition("delivery")) << "Tividy: loadDefinition(name, definition) stores it";

      EXPECT_THROW(([&]
                                          { engine->startInstance("missing"); })(), std::invalid_argument) << "Tividy: startInstance() of an unknown definition is rejected";

      std::string first = engine->startInstance("delivery");
      std::string second = engine->startInstance("outline");
      EXPECT_NE(first, second) << "Tividy: every instance gets its own id";
      EXPECT_TRUE(engine->instance(first).state() == WorkflowInstance::RUNNING) << "Tividy: startInstance() starts the instance";
      EXPECT_TRUE(engine->instance(first).title() == "Feature Delivery") << "Tividy: the instance runs the requested definition";

      std::vector<std::string> ready = engine->readyItems(first);
      EXPECT_TRUE(ready.size() == 2 && ready[0] == "Implement" && ready[1] == "Test") << "Tividy: readyItems() lists the released work in order";
      EXPECT_TRUE(engine->progress(first) == 0.0) << "Tividy: progress() starts at 0";

      WorkflowDefinition changed = deliveryDefinition();
      changed.name = "Feature Delivery v2";
      engine->loadDefinition("delivery", changed);
      EXPECT_TRUE(engine->instance(first).title() == "Feature Delivery") << "Tividy: reloading a definition leaves running instances on their version";
      EXPECT_TRUE(engine->instance(engine->startInstance("delivery")).title() == "Feature Delivery v2") << "Tividy: new instances use the reloaded definition";

      EXPECT_THROW(([&]
                                          { engine->instance("instance-99"); })(), std::invalid_argument) << "Tividy: an unknown instance id is rejected";
      EXPECT_THROW(([&]
                                          { engine->readyItems("instance-99"); })(), std::invalid_argument) << "Tividy: readyItems() of an unknown instance is rejected";
}

// ============================================================================
// TEST: driving work items end to end
// ============================================================================
TEST(Facade, WorkItems) {

      std::unique_ptr<Tividy> engine(makeEngine());
      engine->loadDefinition("delivery", deliveryDefinition());
      std::string id = engine->startInstance("delivery");
      const WorkflowInstance &instance = engine->instance(id);

      engine->assign(id, "Implement", "Alice");
      EXPECT_TRUE(instance.item("Implement")->assignee() == "Alice") << "Tividy: assign() assigns the item";
      EXPECT_EQ(loadOf(*engine, "Alice"), 1) << "Tividy: assign() adds to the assignee's load";
      EXPECT_TRUE(engine->readyItems(id).size() == 1) << "Tividy: assigned work is no longer ready";

      engine->assign(id, "Implement", "Bob");
      EXPECT_TRUE(loadOf(*engine, "Alice") == 0 && loadOf(*engine, "Bob") == 1) << "Tividy: reassigning moves the load to the new assignee";

      EXPECT_THROW(([&]
                                          { engine->assign(id, "Implement", "Nobody"); })(), std::invalid_argument) << "Tividy: assigning to an unknown participant is rejected";
      EXPECT_THROW(([&]
                                          { engine->assign(id, "Ghost", "Alice"); })(), std::invalid_argument) << "Tividy: assigning an unknown item is rejected";
      EXPECT_THROW(([&]
                                          { engine->start(id, "Build"); })(), std::invalid_argument) << "Tividy: a stage id is not accepted as an item";
      EXPECT_THROW(([&]
                                     { engine->complete(id, "Implement"); })(), std::logic_error) << "Tividy: an operation the item's status does not allow is rejected";
      EXPECT_TRUE(instance.item("Implement")->assignee() == "Bob" && loadOf(*engine, "Bob") == 1) << "Tividy: a rejected call changes nothing";

      engine->start(id, "Implement");
      engine->complete(id, "Implement");
      EXPECT_TRUE(instance.item("Implement")->status() == WorkItem::AWAITING_APPROVAL) << "Tividy: start() and complete() move the item to approval";

      engine->reject(id, "Implement", "Needs tests");
      EXPECT_TRUE(instance.item("Implement")->status() == WorkItem::REJECTED) << "Tividy: reject() rejects the work";
      EXPECT_TRUE(instance.item("Implement")->reason() == "Needs tests") << "Tividy: reject() keeps the reason";
      EXPECT_EQ(loadOf(*engine, "Bob"), 0) << "Tividy: reject() releases the assignee's load";

      engine->assign(id, "Implement", "Bob");
      EXPECT_EQ(loadOf(*engine, "Bob"), 1) << "Tividy: rejected work can be reassigned for rework";
      engine->start(id, "Implement");
      engine->complete(id, "Implement");
      engine->approve(id, "Implement");
      EXPECT_TRUE(instance.item("Implement")->isComplete()) << "Tividy: approve() completes the work";
      EXPECT_EQ(loadOf(*engine, "Bob"), 0) << "Tividy: approve() releases the assignee's load";
      EXPECT_TRUE(engine->progress(id) == 0.5) << "Tividy: progress() is the share of finished work items";
      EXPECT_EQ(instance.currentStage(), instance.find("Build")) << "Tividy: the instance stays on a stage with unfinished work";

      engine->assign(id, "Test", "Carol");
      engine->start(id, "Test");
      engine->complete(id, "Test");
      engine->approve(id, "Test");
      EXPECT_EQ(instance.state(), WorkflowInstance::COMPLETED) << "Tividy: approving the last work moves the instance through to Completed";
      EXPECT_TRUE(engine->progress(id) == 1.0) << "Tividy: a completed instance has full progress";
      EXPECT_TRUE(engine->readyItems(id).empty()) << "Tividy: nothing is ready in a completed instance";

      engine->undo();
      engine->undo();

      EXPECT_EQ(instance.state(), WorkflowInstance::RUNNING) << "undo: instance returns to Running after undoing the final approve";
      EXPECT_TRUE(instance.item("Test")->status() == "InProgress") << "undo: Test item rolled back to InProgress";
      EXPECT_TRUE(instance.item("Implement")->status() == "Completed") << "undo: Implement's earlier completion is unaffected";

      // Prove the instance is still usable: drive Test forward again.
      engine->complete(id, "Test");
      EXPECT_TRUE(instance.item("Test")->status() == "AwaitingApproval") << "undo: instance still accepts commands after undo";
      engine->approve(id, "Test");
      EXPECT_EQ(instance.state(), WorkflowInstance::COMPLETED) << "undo: approving again re-completes the instance";
      
      }

// ============================================================================
// TEST: cancelling an instance
// ============================================================================
TEST(Facade, Cancel) {

      std::unique_ptr<Tividy> engine(makeEngine());
      engine->loadDefinition("delivery", deliveryDefinition());
      std::string id = engine->startInstance("delivery");

      engine->assign(id, "Implement", "Alice");
      engine->assign(id, "Test", "Carol");
      engine->start(id, "Test");

      engine->cancel(id, "Feature dropped");
      const WorkflowInstance &instance = engine->instance(id);
      EXPECT_EQ(instance.state(), WorkflowInstance::CANCELLED) << "Tividy: cancel() cancels the instance";
      EXPECT_EQ(instance.reason(), "Feature dropped") << "Tividy: cancel() keeps the reason";
      EXPECT_TRUE(instance.item("Implement")->status() == WorkItem::CANCELLED) << "Tividy: cancel() cancels the instance's work";
      EXPECT_TRUE(loadOf(*engine, "Alice") == 0 && loadOf(*engine, "Carol") == 0) << "Tividy: cancel() releases everyone still working on it";

      EXPECT_THROW(([&]
                                     { engine->cancel(id, "Again"); })(), std::logic_error) << "Tividy: cancelling twice is rejected";
      EXPECT_THROW(([&]
                                     { engine->start(id, "Implement"); })(), std::logic_error) << "Tividy: work in a cancelled instance cannot be started";
      EXPECT_THROW(([&]
                                          { engine->cancel("instance-99"); })(), std::invalid_argument) << "Tividy: cancelling an unknown instance is rejected";
}

// ============================================================================
// MAIN
// ============================================================================

