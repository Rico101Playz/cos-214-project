#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

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

TEST(InstanceState, Names) {
    EXPECT_EQ(CreatedInstance().name(), "Created") << "CreatedInstance: name()";
    EXPECT_EQ(RunningInstance().name(), "Running") << "RunningInstance: name()";
    EXPECT_EQ(CompletedInstance().name(), "Completed") << "CompletedInstance: name()";
    EXPECT_EQ(CancelledInstance().name(), "Cancelled") << "CancelledInstance: name()";
}

TEST(InstanceState, NewInstance) {
    WorkflowInstance inst("inst-1", simpleDefinition());
    EXPECT_EQ(inst.state(), WorkflowInstance::CREATED) << "Created: a new instance starts Created";
    EXPECT_TRUE(inst.id() == "inst-1" && inst.title() == "Onboarding") << "Created: id and title come through";
    EXPECT_TRUE(inst.reason().empty()) << "Created: no cancel reason yet";
    EXPECT_EQ(inst.currentStage(), nullptr) << "Created: there is no current stage before start";
    EXPECT_TRUE(inst.item("a")->status() == "Created" && inst.item("b")->status() == "Created") << "Created: no work is released before start";

    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::CREATED) << "Created: advance() is ignored, not an error";
}

TEST(InstanceState, Start) {
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();

    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Start: Created -> Running";
    EXPECT_EQ(inst.currentStage(), inst.find("Build")) << "Start: the first stage becomes current";
    EXPECT_TRUE(inst.item("a")->status() == "Available" && inst.item("b")->status() == "Available") << "Start: the first stage's work is released";
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ inst.start(); }, "Running")) << "Start: starting a running instance throws, naming the state";
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Start: a refused start changes nothing";
}

TEST(InstanceState, RunningAdvance) {
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();
    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Running: advance() with unfinished work stays Running";

    finishItem(*inst.item("a"));
    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Running: one item still open, still Running";
}

TEST(InstanceState, Completion) {
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();
    finishItem(*inst.item("a"));
    finishItem(*inst.item("b"));
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Completion: finishing the work does not move the instance by itself";

    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::COMPLETED) << "Completion: advance() with all work done -> Completed";
    EXPECT_EQ(inst.currentStage(), nullptr) << "Completion: no current stage once it is Completed";

    EXPECT_TRUE(throwsLogicErrorContaining([&]{ inst.start(); }, "Completed")) << "Completed: start() throws";
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ inst.cancel("too late"); }, "Completed")) << "Completed: cancel() throws";
    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::COMPLETED) << "Completed: advance() is harmless";
}

TEST(InstanceState, CancelFromCreated) {
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.cancel("duplicate request");

    EXPECT_EQ(inst.state(), WorkflowInstance::CANCELLED) << "Cancel: Created -> Cancelled";
    EXPECT_EQ(inst.reason(), "duplicate request") << "Cancel: the reason is stored";
    EXPECT_TRUE(inst.item("a")->status() == "Cancelled" && inst.item("b")->status() == "Cancelled") << "Cancel: all work is cancelled";
    EXPECT_THROW(([&]{ inst.start(); })(), std::logic_error) << "Cancelled: start() throws";
    EXPECT_THROW(([&]{ inst.cancel("again"); })(), std::logic_error) << "Cancelled: cancel() throws";
}

TEST(InstanceState, CancelFromRunning) {
    WorkflowInstance inst("inst-1", simpleDefinition());
    inst.start();
    finishItem(*inst.item("a"));
    inst.cancel("scope change");

    EXPECT_EQ(inst.state(), WorkflowInstance::CANCELLED) << "Cancel: Running -> Cancelled";
    EXPECT_EQ(inst.reason(), "scope change") << "Cancel: the reason is stored";
    EXPECT_TRUE(inst.item("a")->status() == "Completed") << "Cancel: finished work is left as Completed";
    EXPECT_TRUE(inst.item("b")->status() == "Cancelled") << "Cancel: unfinished work is Cancelled";

    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::CANCELLED) << "Cancelled: advance() is ignored";
}

TEST(InstanceState, EmptyAndFlatDefinitions) {
    WorkflowInstance empty("e1", makeDefinition("Empty", {}, {}));
    empty.start();
    EXPECT_EQ(empty.state(), WorkflowInstance::COMPLETED) << "Empty: a definition with no work completes as soon as it starts";

    WorkflowInstance flat("f1", makeDefinition("Flat", {}, {"x"}));
    flat.start();
    EXPECT_EQ(flat.state(), WorkflowInstance::RUNNING) << "Flat: items directly under the root keep it Running";
    EXPECT_EQ(flat.currentStage(), &flat.root()) << "Flat: the root is the current stage";
    EXPECT_TRUE(flat.item("x")->status() == "Available") << "Flat: the item is released";
    finishItem(*flat.item("x"));
    flat.advance();
    EXPECT_EQ(flat.state(), WorkflowInstance::COMPLETED) << "Flat: completes once its work is done";
}

TEST(InstanceState, StageProgression) {
    WorkflowInstance inst("inst-1", makeDefinition("Delivery", {"Build", "Release"}, {"a"}));
    CompositeStage* release = static_cast<CompositeStage*>(inst.find("Release"));
    addLeaf(*release, "b");

    inst.start();
    EXPECT_TRUE(inst.item("a")->status() == "Available") << "Stages: the first stage is released at start";
    EXPECT_TRUE(inst.item("b")->status() == "Created") << "Stages: the second stage stays closed";

    finishItem(*inst.item("a"));
    EXPECT_TRUE(inst.item("b")->status() == "Created") << "Stages: nothing moves on until advance() is called";

    inst.advance();
    EXPECT_EQ(inst.currentStage(), inst.find("Release")) << "Stages: advance() moves to the next stage";
    EXPECT_TRUE(inst.item("b")->status() == "Available") << "Stages: the next stage's work is released";
    EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Stages: still Running with work left";

    finishItem(*inst.item("b"));
    inst.advance();
    EXPECT_EQ(inst.state(), WorkflowInstance::COMPLETED) << "Stages: the last stage finishing completes the instance";
}

TEST(InstanceState, StatesRefuseDirectly) {
    WorkflowInstance inst("inst-1", simpleDefinition());

    CompletedInstance done;
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ done.start(inst); }, "Illegal instance operation")) << "CompletedInstance: start() is refused with the standard message";
    EXPECT_THROW(([&]{ done.cancel(inst, "x"); })(), std::logic_error) << "CompletedInstance: cancel() is refused";

    CancelledInstance gone;
    EXPECT_THROW(([&]{ gone.start(inst); })(), std::logic_error) << "CancelledInstance: start() is refused";
    EXPECT_THROW(([&]{ gone.cancel(inst, "x"); })(), std::logic_error) << "CancelledInstance: cancel() is refused";

    RunningInstance running;
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ running.start(inst); }, "Running")) << "RunningInstance: start() is refused, naming the state";

    EXPECT_EQ(inst.state(), WorkflowInstance::CREATED) << "Refused calls leave the instance untouched";
}
