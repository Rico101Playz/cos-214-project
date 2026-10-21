#include <iostream>
#include <memory>
#include <string>

#include "test_utils.h"
#include "state_test_helpers.h"

#include "core/WorkflowInstance.h"
#include "patterns/execution/CreatedInstance.h"
#include "patterns/execution/RunningInstance.h"
#include "patterns/execution/CompletedInstance.h"
#include "patterns/execution/CancelledInstance.h"

// One stage ("Build") holding two items ("a", "b").
static WorkflowDefinition simpleDefinition() {
    return makeDefinition("Onboarding", {"Build"}, {"a", "b"});
}

static void test_InstanceStateNames() {
    std::cout << "\n--- Testing instance state names ---\n";
    check(CreatedInstance().name() == "Created", "CreatedInstance: name()");
    check(RunningInstance().name() == "Running", "RunningInstance: name()");
    check(CompletedInstance().name() == "Completed", "CompletedInstance: name()");
    check(CancelledInstance().name() == "Cancelled", "CancelledInstance: name()");
}

static void test_NewInstance() {
    std::cout << "\n--- Testing a new instance (Created) ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());
    check(inst.state() == WorkflowInstance::CREATED, "Created: a new instance starts Created");
    check(inst.id() == "inst-1" && inst.title() == "Onboarding", "Created: id and title come through");
    check(inst.reason().empty(), "Created: no cancel reason yet");
    check(inst.currentStage() == nullptr, "Created: there is no current stage before start");
    check(inst.item("a")->status() == "Created" && inst.item("b")->status() == "Created",
          "Created: no work is released before start");

    inst.advance();
    check(inst.state() == WorkflowInstance::CREATED, "Created: advance() is ignored, not an error");
}

static void test_Start() {
    std::cout << "\n--- Testing start() ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();

    check(inst.state() == WorkflowInstance::RUNNING, "Start: Created -> Running");
    check(inst.currentStage() == inst.find("Build"), "Start: the first stage becomes current");
    check(inst.item("a")->status() == "Available" && inst.item("b")->status() == "Available",
          "Start: the first stage's work is released");
    check(throwsLogicErrorContaining([&]{ inst.start(); }, "Running"),
          "Start: starting a running instance throws, naming the state");
    check(inst.state() == WorkflowInstance::RUNNING, "Start: a refused start changes nothing");
}

static void test_RunningAdvance() {
    std::cout << "\n--- Testing advance() while work is unfinished ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();
    inst.advance();
    check(inst.state() == WorkflowInstance::RUNNING, "Running: advance() with unfinished work stays Running");

    finishItem(*inst.item("a"));
    inst.advance();
    check(inst.state() == WorkflowInstance::RUNNING, "Running: one item still open, still Running");
}

static void test_Completion() {
    std::cout << "\n--- Testing completion ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();
    finishItem(*inst.item("a"));
    finishItem(*inst.item("b"));
    check(inst.state() == WorkflowInstance::RUNNING,
          "Completion: finishing the work does not move the instance by itself");

    inst.advance();
    check(inst.state() == WorkflowInstance::COMPLETED, "Completion: advance() with all work done -> Completed");
    check(inst.currentStage() == nullptr, "Completion: no current stage once it is Completed");

    check(throwsLogicErrorContaining([&]{ inst.start(); }, "Completed"), "Completed: start() throws");
    check(throwsLogicErrorContaining([&]{ inst.cancel("too late"); }, "Completed"), "Completed: cancel() throws");
    inst.advance();
    check(inst.state() == WorkflowInstance::COMPLETED, "Completed: advance() is harmless");
}

static void test_CancelFromCreated() {
    std::cout << "\n--- Testing cancel before start ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.cancel("duplicate request");

    check(inst.state() == WorkflowInstance::CANCELLED, "Cancel: Created -> Cancelled");
    check(inst.reason() == "duplicate request", "Cancel: the reason is stored");
    check(inst.item("a")->status() == "Cancelled" && inst.item("b")->status() == "Cancelled",
          "Cancel: all work is cancelled");
    check(throwsLogicError([&]{ inst.start(); }), "Cancelled: start() throws");
    check(throwsLogicError([&]{ inst.cancel("again"); }), "Cancelled: cancel() throws");
}

static void test_CancelFromRunning() {
    std::cout << "\n--- Testing cancel while running ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();
    finishItem(*inst.item("a"));
    inst.cancel("scope change");

    check(inst.state() == WorkflowInstance::CANCELLED, "Cancel: Running -> Cancelled");
    check(inst.reason() == "scope change", "Cancel: the reason is stored");
    check(inst.item("a")->status() == "Completed", "Cancel: finished work is left as Completed");
    check(inst.item("b")->status() == "Cancelled", "Cancel: unfinished work is Cancelled");

    inst.advance();
    check(inst.state() == WorkflowInstance::CANCELLED, "Cancelled: advance() is ignored");
}

static void test_EmptyAndFlatDefinitions() {
    std::cout << "\n--- Testing definitions without stages or without work ---\n";
    WorkflowInstance empty("e1", makeDefinition("Empty", {}, {}));
    empty.start();
    check(empty.state() == WorkflowInstance::COMPLETED, "Empty: a definition with no work completes as soon as it starts");

    WorkflowInstance flat("f1", makeDefinition("Flat", {}, {"x"}));
    flat.start();
    check(flat.state() == WorkflowInstance::RUNNING, "Flat: items directly under the root keep it Running");
    check(flat.currentStage() == &flat.root(), "Flat: the root is the current stage");
    check(flat.item("x")->status() == "Available", "Flat: the item is released");
    finishItem(*flat.item("x"));
    flat.advance();
    check(flat.state() == WorkflowInstance::COMPLETED, "Flat: completes once its work is done");
}

static void test_StageProgression() {
    std::cout << "\n--- Testing movement between stages ---\n";
    WorkflowInstance inst("inst-1", makeDefinition("Delivery", {"Build", "Release"}, {"a"}));
    CompositeStage* release = static_cast<CompositeStage*>(inst.find("Release"));
    addLeaf(*release, "b");

    inst.start();
    check(inst.item("a")->status() == "Available", "Stages: the first stage is released at start");
    check(inst.item("b")->status() == "Created", "Stages: the second stage stays closed");

    finishItem(*inst.item("a"));
    check(inst.item("b")->status() == "Created", "Stages: nothing moves on until advance() is called");

    inst.advance();
    check(inst.currentStage() == inst.find("Release"), "Stages: advance() moves to the next stage");
    check(inst.item("b")->status() == "Available", "Stages: the next stage's work is released");
    check(inst.state() == WorkflowInstance::RUNNING, "Stages: still Running with work left");

    finishItem(*inst.item("b"));
    inst.advance();
    check(inst.state() == WorkflowInstance::COMPLETED, "Stages: the last stage finishing completes the instance");
}

static void test_StatesRefuseDirectly() {
    std::cout << "\n--- Testing state objects directly ---\n";
    WorkflowInstance inst("inst-1", simpleDefinition());

    CompletedInstance done;
    check(throwsLogicErrorContaining([&]{ done.start(inst); }, "Illegal instance operation"),
          "CompletedInstance: start() is refused with the standard message");
    check(throwsLogicError([&]{ done.cancel(inst, "x"); }), "CompletedInstance: cancel() is refused");

    CancelledInstance gone;
    check(throwsLogicError([&]{ gone.start(inst); }), "CancelledInstance: start() is refused");
    check(throwsLogicError([&]{ gone.cancel(inst, "x"); }), "CancelledInstance: cancel() is refused");

    RunningInstance running;
    check(throwsLogicErrorContaining([&]{ running.start(inst); }, "Running"),
          "RunningInstance: start() is refused, naming the state");

    check(inst.state() == WorkflowInstance::CREATED, "Refused calls leave the instance untouched");
}

void run_instance_state_tests() {
    std::cout << "\n--- Running Instance State Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_InstanceStateNames();
    test_NewInstance();
    test_Start();
    test_RunningAdvance();
    test_Completion();
    test_CancelFromCreated();
    test_CancelFromRunning();
    test_EmptyAndFlatDefinitions();
    test_StageProgression();
    test_StatesRefuseDirectly();

    std::cout << "Instance State Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}