#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_utils.h"

#include "core/Participant.h"
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "core/WorkItem.h"
#include "patterns/structural/Tividy.h"

// Runs the action and reports whether it threw the given exception type.
template <typename Exception, typename Action>
static bool throws(Action action) {
    try {
        action();
    } catch (const Exception&) {
        return true;
    }
    return false;
}

// Build -> Review, with the work for Build supplied by the caller.
static WorkflowDefinition deliveryDefinition() {
    WorkflowDefinition definition;
    definition.name = "Feature Delivery";
    definition.stages.push_back("Build");
    definition.stages.push_back("Review");
    definition.items.push_back("Implement");
    definition.items.push_back("Test");
    return definition;
}

static Tividy* makeEngine() {
    Tividy* engine = new Tividy("Acme Software");
    engine->organisation().addParticipant("Alice", "Developer");
    engine->organisation().addParticipant("Bob", "Developer");
    engine->organisation().addParticipant("Carol", "QA");
    return engine;
}

static int loadOf(Tividy& engine, const std::string& name) {
    return engine.organisation().find(name)->load();
}

// ============================================================================
// TEST: definitions and instances
// ============================================================================
void test_FacadeDefinitions() {
    std::cout << "\n--- Testing Tividy definitions and instances ---\n";

    std::unique_ptr<Tividy> engine(makeEngine());
    check(engine->organisation().name() == "Acme Software", "Tividy: owns the named organisation");
    check(engine->hasDefinition("delivery") == false, "Tividy: starts with no definitions");

    engine->loadDefinition("outline");
    check(engine->hasDefinition("outline"), "Tividy: loadDefinition(name) builds and stores a definition");

    engine->loadDefinition("delivery", deliveryDefinition());
    check(engine->hasDefinition("delivery"), "Tividy: loadDefinition(name, definition) stores it");

    check(throws<std::invalid_argument>([&] { engine->startInstance("missing"); }),
          "Tividy: startInstance() of an unknown definition is rejected");

    std::string first = engine->startInstance("delivery");
    std::string second = engine->startInstance("outline");
    check(first != second, "Tividy: every instance gets its own id");
    check(engine->instance(first).state() == WorkflowInstance::RUNNING,
          "Tividy: startInstance() starts the instance");
    check(engine->instance(first).title() == "Feature Delivery",
          "Tividy: the instance runs the requested definition");

    std::vector<std::string> ready = engine->readyItems(first);
    check(ready.size() == 2 && ready[0] == "Implement" && ready[1] == "Test",
          "Tividy: readyItems() lists the released work in order");
    check(engine->progress(first) == 0.0, "Tividy: progress() starts at 0");

    WorkflowDefinition changed = deliveryDefinition();
    changed.name = "Feature Delivery v2";
    engine->loadDefinition("delivery", changed);
    check(engine->instance(first).title() == "Feature Delivery",
          "Tividy: reloading a definition leaves running instances on their version");
    check(engine->instance(engine->startInstance("delivery")).title() == "Feature Delivery v2",
          "Tividy: new instances use the reloaded definition");

    check(throws<std::invalid_argument>([&] { engine->instance("instance-99"); }),
          "Tividy: an unknown instance id is rejected");
    check(throws<std::invalid_argument>([&] { engine->readyItems("instance-99"); }),
          "Tividy: readyItems() of an unknown instance is rejected");
}

// ============================================================================
// TEST: driving work items end to end
// ============================================================================
void test_FacadeWorkItems() {
    std::cout << "\n--- Testing Tividy work item operations ---\n";

    std::unique_ptr<Tividy> engine(makeEngine());
    engine->loadDefinition("delivery", deliveryDefinition());
    std::string id = engine->startInstance("delivery");
    const WorkflowInstance& instance = engine->instance(id);

    engine->assign(id, "Implement", "Alice");
    check(instance.item("Implement")->assignee() == "Alice", "Tividy: assign() assigns the item");
    check(loadOf(*engine, "Alice") == 1, "Tividy: assign() adds to the assignee's load");
    check(engine->readyItems(id).size() == 1, "Tividy: assigned work is no longer ready");

    engine->assign(id, "Implement", "Bob");
    check(loadOf(*engine, "Alice") == 0 && loadOf(*engine, "Bob") == 1,
          "Tividy: reassigning moves the load to the new assignee");

    check(throws<std::invalid_argument>([&] { engine->assign(id, "Implement", "Nobody"); }),
          "Tividy: assigning to an unknown participant is rejected");
    check(throws<std::invalid_argument>([&] { engine->assign(id, "Ghost", "Alice"); }),
          "Tividy: assigning an unknown item is rejected");
    check(throws<std::invalid_argument>([&] { engine->start(id, "Build"); }),
          "Tividy: a stage id is not accepted as an item");
    check(throws<std::logic_error>([&] { engine->complete(id, "Implement"); }),
          "Tividy: an operation the item's status does not allow is rejected");
    check(instance.item("Implement")->assignee() == "Bob" && loadOf(*engine, "Bob") == 1,
          "Tividy: a rejected call changes nothing");

    engine->start(id, "Implement");
    engine->complete(id, "Implement");
    check(instance.item("Implement")->status() == WorkItem::AWAITING_APPROVAL,
          "Tividy: start() and complete() move the item to approval");

    engine->reject(id, "Implement", "Needs tests");
    check(instance.item("Implement")->status() == WorkItem::REJECTED, "Tividy: reject() rejects the work");
    check(instance.item("Implement")->reason() == "Needs tests", "Tividy: reject() keeps the reason");
    check(loadOf(*engine, "Bob") == 0, "Tividy: reject() releases the assignee's load");

    engine->assign(id, "Implement", "Bob");
    check(loadOf(*engine, "Bob") == 1, "Tividy: rejected work can be reassigned for rework");
    engine->start(id, "Implement");
    engine->complete(id, "Implement");
    engine->approve(id, "Implement");
    check(instance.item("Implement")->isComplete(), "Tividy: approve() completes the work");
    check(loadOf(*engine, "Bob") == 0, "Tividy: approve() releases the assignee's load");
    check(engine->progress(id) == 0.5, "Tividy: progress() is the share of finished work items");
    check(instance.currentStage() == instance.find("Build"),
          "Tividy: the instance stays on a stage with unfinished work");

    engine->assign(id, "Test", "Carol");
    engine->start(id, "Test");
    engine->complete(id, "Test");
    engine->approve(id, "Test");
    check(instance.state() == WorkflowInstance::COMPLETED,
          "Tividy: approving the last work moves the instance through to Completed");
    check(engine->progress(id) == 1.0, "Tividy: a completed instance has full progress");
    check(engine->readyItems(id).empty(), "Tividy: nothing is ready in a completed instance");

    engine->undo();
    engine->undo();
    check(instance.state() == WorkflowInstance::COMPLETED,
          "Tividy: undo() goes through the CommandInvoker without corrupting the instance");
}

// ============================================================================
// TEST: cancelling an instance
// ============================================================================
void test_FacadeCancel() {
    std::cout << "\n--- Testing Tividy cancel ---\n";

    std::unique_ptr<Tividy> engine(makeEngine());
    engine->loadDefinition("delivery", deliveryDefinition());
    std::string id = engine->startInstance("delivery");

    engine->assign(id, "Implement", "Alice");
    engine->assign(id, "Test", "Carol");
    engine->start(id, "Test");

    engine->cancel(id, "Feature dropped");
    const WorkflowInstance& instance = engine->instance(id);
    check(instance.state() == WorkflowInstance::CANCELLED, "Tividy: cancel() cancels the instance");
    check(instance.reason() == "Feature dropped", "Tividy: cancel() keeps the reason");
    check(instance.item("Implement")->status() == WorkItem::CANCELLED,
          "Tividy: cancel() cancels the instance's work");
    check(loadOf(*engine, "Alice") == 0 && loadOf(*engine, "Carol") == 0,
          "Tividy: cancel() releases everyone still working on it");

    check(throws<std::logic_error>([&] { engine->cancel(id, "Again"); }),
          "Tividy: cancelling twice is rejected");
    check(throws<std::logic_error>([&] { engine->start(id, "Implement"); }),
          "Tividy: work in a cancelled instance cannot be started");
    check(throws<std::invalid_argument>([&] { engine->cancel("instance-99"); }),
          "Tividy: cancelling an unknown instance is rejected");
}

// ============================================================================
// MAIN
// ============================================================================
void run_facade_tests() {
    std::cout << "\n--- Running Facade (Tividy) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_FacadeDefinitions();
    test_FacadeWorkItems();
    test_FacadeCancel();

    std::cout << "Facade Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
