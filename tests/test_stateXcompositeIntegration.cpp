#include <iostream>
#include <memory>
#include <string>

#include "test_utils.h"
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

static void test_FullDeliveryScenario() {
    std::cout << "\n--- Scenario: a feature delivered from start to finish ---\n";
    WorkflowInstance inst("feat-42", deliveryDefinition());
    buildDelivery(inst);

    check(inst.state() == WorkflowInstance::CREATED && ready(inst).empty(), "Scenario: nothing is ready before start");

    // ---- Start: only the Build stage opens ----
    inst.start();
    check(inst.state() == WorkflowInstance::RUNNING, "Scenario: start() -> Running");
    check(inst.currentStage() == inst.find("Build"), "Scenario: Build is the current stage");
    check(ready(inst) == "implement,test", "Scenario: both Build items are ready, the Release items are not");
    check(throwsLogicError([&]{ inst.item("prod")->assign("x"); }), "Scenario: a locked item cannot be assigned");

    // ---- 'implement': reject, rework, approve ----
    WorkItem* implement = inst.item("implement");
    implement->assign("alice");
    implement->start();
    implement->complete();
    check(implement->status() == "AwaitingApproval" && ready(inst) == "test", "Scenario: implement awaits approval");

    implement->reject("missing edge cases");
    check(implement->status() == "Rejected" && implement->reason() == "missing edge cases", "Scenario: implement is rejected with a reason");
    inst.advance();
    check(inst.state() == WorkflowInstance::RUNNING && !inst.find("Build")->isComplete(), "Scenario: a rejection keeps Build open");

    implement->assign("bob");
    check(implement->reason().empty(), "Scenario: reassigning clears the rejection reason");
    implement->start();
    implement->complete();
    implement->approve();
    check(implement->status() == "Completed" && nearly(inst.find("Build")->progress(), 0.5), "Scenario: rework approved, Build is 50%");

    // ---- 'test': escalate, reassign, finish ----
    LeafWorkItem* test = static_cast<LeafWorkItem*>(inst.item("test"));
    test->assign("carol");
    test->escalate();
    check(test->status() == "Escalated" && ready(inst).empty(), "Scenario: an escalated item is not ready for anyone");
    test->assign("dave");
    check(test->status() == "Assigned" && test->assignee == "dave", "Scenario: escalation ends with reassignment");
    test->start();
    test->complete();
    test->approve();


    check(inst.find("Build")->isComplete(), "Scenario: Build is complete");
    check(inst.state() == WorkflowInstance::RUNNING && inst.currentStage() == inst.find("Build"),
          "Scenario: the instance only moves on when advance() is called");
    check(inst.item("review")->status() == "Created", "Scenario: Release is still locked");

    // ---- advance(): Release opens, sequentially ----
    inst.advance();
    check(inst.currentStage() == inst.find("Release"), "Scenario: advance() moves to Release");
    check(ready(inst) == "review", "Scenario: only 'review' is ready, 'prod' waits behind it");
    check(inst.item("prod")->status() == "Created", "Scenario: the nested deploy stage stays locked");

    // ---- review completes -> deploy unlocks without any advance() ----
    finishItem(*inst.item("review"));
    check(inst.item("prod")->status() == "Available", "Scenario: finishing 'review' unlocks the deploy stage");
    check(ready(inst) == "prod", "Scenario: 'prod' is now the only ready item");
    check(inst.state() == WorkflowInstance::RUNNING, "Scenario: still Running with work left");

    // ---- finish ----
    finishItem(*inst.item("prod"));
    check(inst.root().isComplete() && nearly(inst.root().progress(), 1.0), "Scenario: the whole tree reports done");
    check(inst.state() == WorkflowInstance::RUNNING, "Scenario: the instance is Running until advance()");
    inst.advance();
    check(inst.state() == WorkflowInstance::COMPLETED, "Scenario: advance() -> Completed");
    check(ready(inst).empty(), "Scenario: nothing is left to pick up");
    check(throwsLogicError([&]{ inst.cancel("too late"); }), "Scenario: a completed instance cannot be cancelled");
}

static void test_CancelMidwayScenario() {
    std::cout << "\n--- Scenario: cancelled halfway through ---\n";
    WorkflowInstance inst("feat-43", deliveryDefinition());
    buildDelivery(inst);
    inst.start();
    finishItem(*inst.item("implement"));

    inst.cancel("scope change");
    check(inst.state() == WorkflowInstance::CANCELLED && inst.reason() == "scope change", "Cancel: instance Cancelled with its reason");
    check(inst.item("implement")->status() == "Completed", "Cancel: finished work is kept");
    check(inst.item("test")->status() == "Cancelled", "Cancel: open work in the current stage is cancelled");
    check(inst.item("review")->status() == "Cancelled" && inst.item("prod")->status() == "Cancelled",
          "Cancel: work in later and nested stages is cancelled too");
    check(ready(inst).empty(), "Cancel: nothing remains to pick up");

    inst.advance();
    check(inst.state() == WorkflowInstance::CANCELLED, "Cancel: advance() cannot revive it");
    check(throwsLogicError([&]{ inst.start(); }) && throwsLogicError([&]{ inst.cancel("again"); }),
          "Cancel: start() and cancel() are refused afterwards");
    check(throwsLogicError([&]{ inst.item("test")->assign("x"); }), "Cancel: cancelled items accept no work");
}

static void test_CancelBeforeStartScenario() {
    std::cout << "\n--- Scenario: cancelled before it ever started ---\n";
    WorkflowInstance inst("feat-44", deliveryDefinition());
    buildDelivery(inst);
    inst.cancel("not needed");

    check(inst.state() == WorkflowInstance::CANCELLED, "Early cancel: instance Cancelled");
    check(inst.item("implement")->status() == "Cancelled" && inst.item("prod")->status() == "Cancelled",
          "Early cancel: every item in the tree is Cancelled");
    check(throwsLogicError([&]{ inst.start(); }), "Early cancel: it can never be started");
}

static void test_InstancesAreIndependent() {
    std::cout << "\n--- Scenario: two instances from the same definition ---\n";
    WorkflowDefinition def = deliveryDefinition();
    WorkflowInstance one("one", def);
    WorkflowInstance two("two", def);

    one.start();
    finishItem(*one.item("implement"));

    check(two.state() == WorkflowInstance::CREATED, "Independent: starting one does not start the other");
    check(two.item("implement")->status() == "Created", "Independent: the other's items are untouched");

    two.start();
    check(two.item("implement")->status() == "Available", "Independent: the second runs from scratch");
    check(one.item("implement")->status() == "Completed", "Independent: the first keeps its own progress");
}

void run_state_composite_integration_tests() {
    std::cout << "\n--- Running State + Composite Integration Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_FullDeliveryScenario();
    test_CancelMidwayScenario();
    test_CancelBeforeStartScenario();
    test_InstancesAreIndependent();

    std::cout << "State + Composite Integration Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}