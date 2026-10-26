#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "test_utils.h"

#include "core/WorkItem.h"
#include "patterns/structural/WorkNode.h"
#include "patterns/structural/CompositeStage.h"

// Runs the action and reports whether it threw std::logic_error.
template <typename Action>
static bool throwsLogicError(Action action) {
    try {
        action();
    } catch (const std::logic_error&) {
        return true;
    }
    return false;
}

// ============================================================================
// TEST: WorkItem lifecycle
// ============================================================================
void test_WorkItemLifecycle() {
    std::cout << "\n--- Testing WorkItem lifecycle ---\n";

    WorkItem item("F06", "Implement backend", "Developer");
    check(item.status() == WorkItem::CREATED, "WorkItem: starts Created");
    check(item.isReady() == false, "WorkItem: a Created item is not ready");
    check(item.isComplete() == false, "WorkItem: a Created item is not complete");
    check(item.progress() == 0.0, "WorkItem: progress is 0 before completion");

    check(throwsLogicError([&] { item.start(); }),
          "WorkItem: start() before assignment is rejected");
    check(item.status() == WorkItem::CREATED, "WorkItem: a rejected move leaves the status unchanged");

    item.makeAvailable();
    check(item.status() == WorkItem::AVAILABLE, "WorkItem: makeAvailable() moves it to Available");
    check(item.isReady() == true, "WorkItem: an Available item is ready");

    item.assign("Alice");
    check(item.status() == WorkItem::ASSIGNED, "WorkItem: assign() moves it to Assigned");
    check(item.assignee() == "Alice", "WorkItem: assign() records the assignee");
    check(item.isReady() == false, "WorkItem: an Assigned item is no longer ready");

    item.assign("Bob");
    check(item.assignee() == "Bob", "WorkItem: an Assigned item can be reassigned");

    item.start();
    check(item.status() == WorkItem::IN_PROGRESS, "WorkItem: start() moves it to InProgress");
    item.complete();
    check(item.status() == WorkItem::AWAITING_APPROVAL, "WorkItem: complete() waits for approval");
    check(item.isComplete() == false, "WorkItem: an item awaiting approval is not complete yet");

    item.reject("Missing tests");
    check(item.status() == WorkItem::REJECTED, "WorkItem: reject() moves it to Rejected");
    check(item.reason() == "Missing tests", "WorkItem: reject() records the reason");

    item.assign("Bob");
    check(item.status() == WorkItem::ASSIGNED, "WorkItem: rejected work can be reassigned for rework");
    check(item.reason() == "", "WorkItem: reassigning clears the rejection reason");

    item.start();
    item.complete();
    item.approve();
    check(item.status() == WorkItem::COMPLETED, "WorkItem: approve() moves it to Completed");
    check(item.isComplete() == true, "WorkItem: a Completed item is complete");
    check(item.progress() == 1.0, "WorkItem: progress is 1 once complete");

    item.cancel();
    check(item.status() == WorkItem::COMPLETED, "WorkItem: cancel() leaves completed work alone");

    WorkItem other("F07", "Write docs", "Developer");
    other.cancel();
    check(other.status() == WorkItem::CANCELLED, "WorkItem: cancel() cancels unfinished work");
    check(throwsLogicError([&] { other.makeAvailable(); }),
          "WorkItem: a cancelled item cannot be released again");
}

// ============================================================================
// TEST: CompositeStage
// ============================================================================
void test_CompositeStage() {
    std::cout << "\n--- Testing CompositeStage ---\n";

    CompositeStage build("build", "Build");
    check(build.childCount() == 0, "CompositeStage: starts with no children");
    check(build.isComplete() == true, "CompositeStage: an empty stage is complete");
    check(build.progress() == 1.0, "CompositeStage: an empty stage has full progress");
    check(build.isReady() == false, "CompositeStage: a stage is never ready itself");

    WorkNode& implement = build.add(std::unique_ptr<WorkNode>(
        new WorkItem("implement", "Implement feature", "Developer")));
    build.add(std::unique_ptr<WorkNode>(new WorkItem("test", "Write tests", "QA")));

    check(build.childCount() == 2, "CompositeStage: add() stores children");
    check(build.child(0) == &implement, "CompositeStage: children keep the order they were added");
    check(build.child(5) == nullptr, "CompositeStage: child() out of range is nullptr");
    check(implement.parent() == &build, "CompositeStage: add() sets the parent of the child");
    check(build.parent() == nullptr, "CompositeStage: a root has no parent");

    bool threw = false;
    try {
        build.add(std::unique_ptr<WorkNode>());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "CompositeStage: a null child is rejected");

    check(build.find("test") == build.child(1), "CompositeStage: find() locates a child by id");
    check(build.find("build") == &build, "CompositeStage: find() matches the stage itself");
    check(build.find("nope") == nullptr, "CompositeStage: find() returns nullptr for an unknown id");

    WorkItem& first = static_cast<WorkItem&>(implement);
    first.makeAvailable();
    first.assign("Alice");
    first.start();
    first.complete();
    first.approve();
    check(build.isComplete() == false, "CompositeStage: incomplete while any child is incomplete");
    check(build.progress() == 0.5, "CompositeStage: progress is the average of its children");

    CompositeStage root("delivery", "Feature Delivery");
    root.add(std::unique_ptr<WorkNode>(new CompositeStage("release", "Release")));
    CompositeStage* release = static_cast<CompositeStage*>(root.child(0));
    release->add(std::unique_ptr<WorkNode>(new WorkItem("deploy", "Deploy", "ReleaseManager")));
    check(root.find("deploy") != nullptr, "CompositeStage: find() searches nested stages");

    root.cancel();
    WorkItem* deploy = static_cast<WorkItem*>(root.find("deploy"));
    check(deploy->status() == WorkItem::CANCELLED, "CompositeStage: cancel() cascades to nested work");

    check(build.remove("test") == true, "CompositeStage: remove() drops a direct child");
    check(build.childCount() == 1, "CompositeStage: remove() shrinks the stage");
    check(build.remove("test") == false, "CompositeStage: remove() of a missing child returns false");
    check(build.isComplete() == true, "CompositeStage: complete once only finished children remain");
}

// ============================================================================
// MAIN
// ============================================================================
void run_composite_tests() {
    std::cout << "\n--- Running Composite (work tree) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_WorkItemLifecycle();
    test_CompositeStage();

    std::cout << "Composite Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
