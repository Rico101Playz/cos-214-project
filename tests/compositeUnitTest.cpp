#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>


#include "composite/LeafTask.h"
#include "composite/Task.h"
#include "composite/CompositeTask.h"

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
// TEST: LeafTask lifecycle
// ============================================================================
TEST(Composite, WorkItemLifecycle) {

    LeafTask item("F06", "Implement backend", "Developer");
    EXPECT_EQ(item.status(), LeafTask::CREATED) << "LeafTask: starts Created";
    EXPECT_FALSE(item.isReady()) << "LeafTask: a Created item is not ready";
    EXPECT_FALSE(item.isComplete()) << "LeafTask: a Created item is not complete";
    EXPECT_EQ(item.progress(), 0.0) << "LeafTask: progress is 0 before completion";

    EXPECT_THROW(([&] { item.start(); })(), std::logic_error) << "LeafTask: start() before assignment is rejected";
    EXPECT_EQ(item.status(), LeafTask::CREATED) << "LeafTask: a rejected move leaves the status unchanged";

    item.makeAvailable();
    EXPECT_EQ(item.status(), LeafTask::AVAILABLE) << "LeafTask: makeAvailable() moves it to Available";
    EXPECT_TRUE(item.isReady()) << "LeafTask: an Available item is ready";

    item.assign("Alice");
    EXPECT_EQ(item.status(), LeafTask::ASSIGNED) << "LeafTask: assign() moves it to Assigned";
    EXPECT_EQ(item.assignee(), "Alice") << "LeafTask: assign() records the assignee";
    EXPECT_FALSE(item.isReady()) << "LeafTask: an Assigned item is no longer ready";

    item.assign("Bob");
    EXPECT_EQ(item.assignee(), "Bob") << "LeafTask: an Assigned item can be reassigned";

    item.start();
    EXPECT_EQ(item.status(), LeafTask::IN_PROGRESS) << "LeafTask: start() moves it to InProgress";
    item.complete();
    EXPECT_EQ(item.status(), LeafTask::AWAITING_APPROVAL) << "LeafTask: complete() waits for approval";
    EXPECT_FALSE(item.isComplete()) << "LeafTask: an item awaiting approval is not complete yet";

    item.reject("Missing tests");
    EXPECT_EQ(item.status(), LeafTask::REJECTED) << "LeafTask: reject() moves it to Rejected";
    EXPECT_EQ(item.reason(), "Missing tests") << "LeafTask: reject() records the reason";

    item.assign("Bob");
    EXPECT_EQ(item.status(), LeafTask::ASSIGNED) << "LeafTask: rejected work can be reassigned for rework";
    EXPECT_EQ(item.reason(), "") << "LeafTask: reassigning clears the rejection reason";

    item.start();
    item.complete();
    item.approve();
    EXPECT_EQ(item.status(), LeafTask::COMPLETED) << "LeafTask: approve() moves it to Completed";
    EXPECT_TRUE(item.isComplete()) << "LeafTask: a Completed item is complete";
    EXPECT_EQ(item.progress(), 1.0) << "LeafTask: progress is 1 once complete";

    item.cancel();
    EXPECT_EQ(item.status(), LeafTask::COMPLETED) << "LeafTask: cancel() leaves completed work alone";

    LeafTask other("F07", "Write docs", "Developer");
    other.cancel();
    EXPECT_EQ(other.status(), LeafTask::CANCELLED) << "LeafTask: cancel() cancels unfinished work";
    EXPECT_THROW(([&] { other.makeAvailable(); })(), std::logic_error) << "LeafTask: a cancelled item cannot be released again";
}

// ============================================================================
// TEST: CompositeTask
// ============================================================================
TEST(Composite, Stage) {

    CompositeTask build("build", "Build");
    EXPECT_EQ(build.childCount(), 0u) << "CompositeTask: starts with no children";
    EXPECT_TRUE(build.isComplete()) << "CompositeTask: an empty stage is complete";
    EXPECT_EQ(build.progress(), 1.0) << "CompositeTask: an empty stage has full progress";
    EXPECT_FALSE(build.isReady()) << "CompositeTask: a stage is never ready itself";

    Task& implement = build.add(std::unique_ptr<Task>(
        new LeafTask("implement", "Implement feature", "Developer")));
    build.add(std::unique_ptr<Task>(new LeafTask("test", "Write tests", "QA")));

    EXPECT_EQ(build.childCount(), 2u) << "CompositeTask: add() stores children";
    EXPECT_EQ(build.child(0), &implement) << "CompositeTask: children keep the order they were added";
    EXPECT_EQ(build.child(5), nullptr) << "CompositeTask: child() out of range is nullptr";
    EXPECT_EQ(implement.parent(), &build) << "CompositeTask: add() sets the parent of the child";
    EXPECT_EQ(build.parent(), nullptr) << "CompositeTask: a root has no parent";

    bool threw = false;
    try {
        build.add(std::unique_ptr<Task>());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT_TRUE(threw) << "CompositeTask: a null child is rejected";

    EXPECT_EQ(build.find("test"), build.child(1)) << "CompositeTask: find() locates a child by id";
    EXPECT_EQ(build.find("build"), &build) << "CompositeTask: find() matches the stage itself";
    EXPECT_EQ(build.find("nope"), nullptr) << "CompositeTask: find() returns nullptr for an unknown id";

    LeafTask& first = static_cast<LeafTask&>(implement);
    first.makeAvailable();
    first.assign("Alice");
    first.start();
    first.complete();
    first.approve();
    EXPECT_FALSE(build.isComplete()) << "CompositeTask: incomplete while any child is incomplete";
    EXPECT_EQ(build.progress(), 0.5) << "CompositeTask: progress is the average of its children";

    CompositeTask root("delivery", "Feature Delivery");
    root.add(std::unique_ptr<Task>(new CompositeTask("release", "Release")));
    CompositeTask* release = static_cast<CompositeTask*>(root.child(0));
    release->add(std::unique_ptr<Task>(new LeafTask("deploy", "Deploy", "ReleaseManager")));
    EXPECT_NE(root.find("deploy"), nullptr) << "CompositeTask: find() searches nested stages";

    root.cancel();
    LeafTask* deploy = static_cast<LeafTask*>(root.find("deploy"));
    EXPECT_TRUE(deploy->status() == LeafTask::CANCELLED) << "CompositeTask: cancel() cascades to nested work";

    EXPECT_TRUE(build.remove("test")) << "CompositeTask: remove() drops a direct child";
    EXPECT_EQ(build.childCount(), 1u) << "CompositeTask: remove() shrinks the stage";
    EXPECT_FALSE(build.remove("test")) << "CompositeTask: remove() of a missing child returns false";
    EXPECT_TRUE(build.isComplete()) << "CompositeTask: complete once only finished children remain";
}

// ============================================================================
// MAIN
// ============================================================================

