#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>


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
TEST(Composite, WorkItemLifecycle) {

    WorkItem item("F06", "Implement backend", "Developer");
    EXPECT_EQ(item.status(), WorkItem::CREATED) << "WorkItem: starts Created";
    EXPECT_FALSE(item.isReady()) << "WorkItem: a Created item is not ready";
    EXPECT_FALSE(item.isComplete()) << "WorkItem: a Created item is not complete";
    EXPECT_EQ(item.progress(), 0.0) << "WorkItem: progress is 0 before completion";

    EXPECT_THROW(([&] { item.start(); })(), std::logic_error) << "WorkItem: start() before assignment is rejected";
    EXPECT_EQ(item.status(), WorkItem::CREATED) << "WorkItem: a rejected move leaves the status unchanged";

    item.makeAvailable();
    EXPECT_EQ(item.status(), WorkItem::AVAILABLE) << "WorkItem: makeAvailable() moves it to Available";
    EXPECT_TRUE(item.isReady()) << "WorkItem: an Available item is ready";

    item.assign("Alice");
    EXPECT_EQ(item.status(), WorkItem::ASSIGNED) << "WorkItem: assign() moves it to Assigned";
    EXPECT_EQ(item.assignee(), "Alice") << "WorkItem: assign() records the assignee";
    EXPECT_FALSE(item.isReady()) << "WorkItem: an Assigned item is no longer ready";

    item.assign("Bob");
    EXPECT_EQ(item.assignee(), "Bob") << "WorkItem: an Assigned item can be reassigned";

    item.start();
    EXPECT_EQ(item.status(), WorkItem::IN_PROGRESS) << "WorkItem: start() moves it to InProgress";
    item.complete();
    EXPECT_EQ(item.status(), WorkItem::AWAITING_APPROVAL) << "WorkItem: complete() waits for approval";
    EXPECT_FALSE(item.isComplete()) << "WorkItem: an item awaiting approval is not complete yet";

    item.reject("Missing tests");
    EXPECT_EQ(item.status(), WorkItem::REJECTED) << "WorkItem: reject() moves it to Rejected";
    EXPECT_EQ(item.reason(), "Missing tests") << "WorkItem: reject() records the reason";

    item.assign("Bob");
    EXPECT_EQ(item.status(), WorkItem::ASSIGNED) << "WorkItem: rejected work can be reassigned for rework";
    EXPECT_EQ(item.reason(), "") << "WorkItem: reassigning clears the rejection reason";

    item.start();
    item.complete();
    item.approve();
    EXPECT_EQ(item.status(), WorkItem::COMPLETED) << "WorkItem: approve() moves it to Completed";
    EXPECT_TRUE(item.isComplete()) << "WorkItem: a Completed item is complete";
    EXPECT_EQ(item.progress(), 1.0) << "WorkItem: progress is 1 once complete";

    item.cancel();
    EXPECT_EQ(item.status(), WorkItem::COMPLETED) << "WorkItem: cancel() leaves completed work alone";

    WorkItem other("F07", "Write docs", "Developer");
    other.cancel();
    EXPECT_EQ(other.status(), WorkItem::CANCELLED) << "WorkItem: cancel() cancels unfinished work";
    EXPECT_THROW(([&] { other.makeAvailable(); })(), std::logic_error) << "WorkItem: a cancelled item cannot be released again";
}

// ============================================================================
// TEST: CompositeStage
// ============================================================================
TEST(Composite, Stage) {

    CompositeStage build("build", "Build");
    EXPECT_EQ(build.childCount(), 0u) << "CompositeStage: starts with no children";
    EXPECT_TRUE(build.isComplete()) << "CompositeStage: an empty stage is complete";
    EXPECT_EQ(build.progress(), 1.0) << "CompositeStage: an empty stage has full progress";
    EXPECT_FALSE(build.isReady()) << "CompositeStage: a stage is never ready itself";

    WorkNode& implement = build.add(std::unique_ptr<WorkNode>(
        new WorkItem("implement", "Implement feature", "Developer")));
    build.add(std::unique_ptr<WorkNode>(new WorkItem("test", "Write tests", "QA")));

    EXPECT_EQ(build.childCount(), 2u) << "CompositeStage: add() stores children";
    EXPECT_EQ(build.child(0), &implement) << "CompositeStage: children keep the order they were added";
    EXPECT_EQ(build.child(5), nullptr) << "CompositeStage: child() out of range is nullptr";
    EXPECT_EQ(implement.parent(), &build) << "CompositeStage: add() sets the parent of the child";
    EXPECT_EQ(build.parent(), nullptr) << "CompositeStage: a root has no parent";

    bool threw = false;
    try {
        build.add(std::unique_ptr<WorkNode>());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT_TRUE(threw) << "CompositeStage: a null child is rejected";

    EXPECT_EQ(build.find("test"), build.child(1)) << "CompositeStage: find() locates a child by id";
    EXPECT_EQ(build.find("build"), &build) << "CompositeStage: find() matches the stage itself";
    EXPECT_EQ(build.find("nope"), nullptr) << "CompositeStage: find() returns nullptr for an unknown id";

    WorkItem& first = static_cast<WorkItem&>(implement);
    first.makeAvailable();
    first.assign("Alice");
    first.start();
    first.complete();
    first.approve();
    EXPECT_FALSE(build.isComplete()) << "CompositeStage: incomplete while any child is incomplete";
    EXPECT_EQ(build.progress(), 0.5) << "CompositeStage: progress is the average of its children";

    CompositeStage root("delivery", "Feature Delivery");
    root.add(std::unique_ptr<WorkNode>(new CompositeStage("release", "Release")));
    CompositeStage* release = static_cast<CompositeStage*>(root.child(0));
    release->add(std::unique_ptr<WorkNode>(new WorkItem("deploy", "Deploy", "ReleaseManager")));
    EXPECT_NE(root.find("deploy"), nullptr) << "CompositeStage: find() searches nested stages";

    root.cancel();
    WorkItem* deploy = static_cast<WorkItem*>(root.find("deploy"));
    EXPECT_TRUE(deploy->status() == WorkItem::CANCELLED) << "CompositeStage: cancel() cascades to nested work";

    EXPECT_TRUE(build.remove("test")) << "CompositeStage: remove() drops a direct child";
    EXPECT_EQ(build.childCount(), 1u) << "CompositeStage: remove() shrinks the stage";
    EXPECT_FALSE(build.remove("test")) << "CompositeStage: remove() of a missing child returns false";
    EXPECT_TRUE(build.isComplete()) << "CompositeStage: complete once only finished children remain";
}

// ============================================================================
// MAIN
// ============================================================================

