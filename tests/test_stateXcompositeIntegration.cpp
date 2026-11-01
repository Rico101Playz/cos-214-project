#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

#include "state_test_helpers.h"

#include "core/WorkflowInstance.h"
#include "patterns/execution/SequentialMode.h"
#include "patterns/execution/ReadyOnlyIterator.h"

// Feature Delivery:
//   Build   (parallel):   implement, test
//   Release (sequential): review, then deploy { prod }
static void buildDelivery(WorkflowInstance& inst) {
    CompositeStage* release = static_cast<CompositeStage*>(inst.find("Release"));
    release->setMode(std::unique_ptr<StageMode>(new SequentialMode()));
    addLeaf(*release, "review", "Reviewer");
    CompositeStage* deploy = addSubStage(*release, "deploy");
    addLeaf(*deploy, "prod", "ReleaseManager");
}

static WorkflowDefinition deliveryDefinition() {
    return makeDefinition("Feature Delivery", {"Build", "Release"}, {"implement", "test"});
}

// Ids of everything that is Available right now, found by walking the live tree.
static std::string ready(WorkflowInstance& inst) {
    ReadyOnlyIterator it(&inst.root());
    return walkIds(it);
}

TEST(StateCompositeIntegration, FullDeliveryScenario) {
    WorkflowInstance inst("feat-42", deliveryDefinition());
    buildDelivery(inst);

    EXPECT_TRUE(inst.state() == WorkflowInstance::CREATED && ready(inst).empty()) << "Scenario: nothing is ready before start";

    // ---- Start: only the Build stage opens ----
    inst.start();
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Scenario: start() -> Running";
    EXPECT_EQ(inst.currentStage(), inst.find("Build")) << "Scenario: Build is the current stage";
    EXPECT_EQ(ready(inst), "implement,test") << "Scenario: both Build items are ready, the Release items are not";
    EXPECT_THROW(([&]{ inst.item("prod")->assign("x"); })(), std::logic_error) << "Scenario: a locked item cannot be assigned";

    // ---- 'implement': reject, rework, approve ----
    WorkItem* implement = inst.item("implement");
    implement->assign("alice");
    implement->start();
    implement->complete();
    EXPECT_TRUE(implement->status() == "AwaitingApproval" && ready(inst) == "test") << "Scenario: implement awaits approval";

    implement->reject("missing edge cases");
    EXPECT_TRUE(implement->status() == "Rejected" && implement->reason() == "missing edge cases") << "Scenario: implement is rejected with a reason";
    inst.advance();
    EXPECT_TRUE(inst.state() == WorkflowInstance::RUNNING && !inst.find("Build")->isComplete()) << "Scenario: a rejection keeps Build open";

    implement->assign("bob");
    EXPECT_TRUE(implement->reason().empty()) << "Scenario: reassigning clears the rejection reason";
    implement->start();
    implement->complete();
    implement->approve();
    EXPECT_TRUE(implement->status() == "Completed" && nearly(inst.find("Build")->progress(), 0.5)) << "Scenario: rework approved, Build is 50%";

    // ---- 'test': escalate, reassign, finish ----
    LeafWorkItem* test = static_cast<LeafWorkItem*>(inst.item("test"));
    test->assign("carol");
    test->escalate();
    EXPECT_TRUE(test->status() == "Escalated" && ready(inst).empty()) << "Scenario: an escalated item is not ready for anyone";
    test->assign("dave");
    EXPECT_TRUE(test->status() == "Assigned" && test->assignee() == "dave") << "Scenario: escalation ends with reassignment";
    test->start();
    test->complete();
    test->approve();


    EXPECT_TRUE(inst.find("Build")->isComplete()) << "Scenario: Build is complete";
    EXPECT_TRUE(inst.state() == WorkflowInstance::RUNNING && inst.currentStage() == inst.find("Build")) << "Scenario: the instance only moves on when advance() is called";
    EXPECT_TRUE(inst.item("review")->status() == "Created") << "Scenario: Release is still locked";

    // ---- advance(): Release opens, sequentially ----
    inst.advance();
    EXPECT_EQ(inst.currentStage(), inst.find("Release")) << "Scenario: advance() moves to Release";
    EXPECT_EQ(ready(inst), "review") << "Scenario: only 'review' is ready, 'prod' waits behind it";
    EXPECT_TRUE(inst.item("prod")->status() == "Created") << "Scenario: the nested deploy stage stays locked";

    // ---- review completes -> deploy unlocks without any advance() ----
    finishItem(*inst.item("review"));
    EXPECT_TRUE(inst.item("prod")->status() == "Available") << "Scenario: finishing 'review' unlocks the deploy stage";
    EXPECT_EQ(ready(inst), "prod") << "Scenario: 'prod' is now the only ready item";
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Scenario: still Running with work left";

    // ---- finish ----
    finishItem(*inst.item("prod"));
    EXPECT_TRUE(inst.root().isComplete() && nearly(inst.root().progress(), 1.0)) << "Scenario: the whole tree reports done";
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Scenario: the instance is Running until advance()";
    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::COMPLETED) << "Scenario: advance() -> Completed";
    EXPECT_TRUE(ready(inst).empty()) << "Scenario: nothing is left to pick up";
    EXPECT_THROW(([&]{ inst.cancel("too late"); })(), std::logic_error) << "Scenario: a completed instance cannot be cancelled";
}

TEST(StateCompositeIntegration, CancelMidwayScenario) {
    WorkflowInstance inst("feat-43", deliveryDefinition());
    buildDelivery(inst);
    inst.start();
    finishItem(*inst.item("implement"));

    inst.cancel("scope change");
    EXPECT_TRUE(inst.state() == WorkflowInstance::CANCELLED && inst.reason() == "scope change") << "Cancel: instance Cancelled with its reason";
    EXPECT_TRUE(inst.item("implement")->status() == "Completed") << "Cancel: finished work is kept";
    EXPECT_TRUE(inst.item("test")->status() == "Cancelled") << "Cancel: open work in the current stage is cancelled";
    EXPECT_TRUE(inst.item("review")->status() == "Cancelled" && inst.item("prod")->status() == "Cancelled") << "Cancel: work in later and nested stages is cancelled too";
    EXPECT_TRUE(ready(inst).empty()) << "Cancel: nothing remains to pick up";

    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::CANCELLED) << "Cancel: advance() cannot revive it";
    EXPECT_TRUE(throwsLogicError([&]{ inst.start(); }) && throwsLogicError([&]{ inst.cancel("again"); })) << "Cancel: start() and cancel() are refused afterwards";
    EXPECT_THROW(([&]{ inst.item("test")->assign("x"); })(), std::logic_error) << "Cancel: cancelled items accept no work";
}

TEST(StateCompositeIntegration, CancelBeforeStartScenario) {
    WorkflowInstance inst("feat-44", deliveryDefinition());
    buildDelivery(inst);
    inst.cancel("not needed");

    EXPECT_EQ(inst.state(), WorkflowInstance::CANCELLED) << "Early cancel: instance Cancelled";
    EXPECT_TRUE(inst.item("implement")->status() == "Cancelled" && inst.item("prod")->status() == "Cancelled") << "Early cancel: every item in the tree is Cancelled";
    EXPECT_THROW(([&]{ inst.start(); })(), std::logic_error) << "Early cancel: it can never be started";
}

TEST(StateCompositeIntegration, InstancesAreIndependent) {
    WorkflowDefinition def = deliveryDefinition();
    WorkflowInstance one("one", def);
    WorkflowInstance two("two", def);

    one.start();
    finishItem(*one.item("implement"));

    EXPECT_EQ(two.state(), WorkflowInstance::CREATED) << "Independent: starting one does not start the other";
    EXPECT_TRUE(two.item("implement")->status() == "Created") << "Independent: the other's items are untouched";

    two.start();
    EXPECT_TRUE(two.item("implement")->status() == "Available") << "Independent: the second runs from scratch";
    EXPECT_TRUE(one.item("implement")->status() == "Completed") << "Independent: the first keeps its own progress";
}
