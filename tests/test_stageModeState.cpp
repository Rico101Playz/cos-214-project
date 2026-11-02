#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

#include "state_test_helpers.h"

#include "strategy/SequentialMode.h"
#include "strategy/ParallelMode.h"

static void useSequential(CompositeTask& stage) {
    stage.setMode(std::unique_ptr<StageMode>(new SequentialMode()));
}

TEST(StageMode, ModeNames) {
    EXPECT_EQ(SequentialMode().name(), "Sequential") << "SequentialMode: name()";
    EXPECT_EQ(ParallelMode().name(), "Parallel") << "ParallelMode: name()";

    CompositeTask stage("s", "Stage");
    EXPECT_EQ(stage.mode().name(), "Parallel") << "CompositeTask: defaults to Parallel";
    useSequential(stage);
    EXPECT_EQ(stage.mode().name(), "Sequential") << "CompositeTask: setMode() switches the mode";
    stage.setMode(std::unique_ptr<StageMode>());
    EXPECT_EQ(stage.mode().name(), "Parallel") << "CompositeTask: clearing the mode falls back to Parallel";
}

TEST(StageMode, ParallelMode) {
    CompositeTask stage("s", "Stage");
    LeafTask* a = addLeaf(stage, "a");
    LeafTask* b = addLeaf(stage, "b");
    LeafTask* c = addLeaf(stage, "c");

    stage.activate();
    EXPECT_TRUE(a->status() == "Available" && b->status() == "Available" && c->status() == "Available") << "Parallel: activate() releases every child at once";

    finishItem(*a);
    EXPECT_TRUE(b->status() == "Available" && c->status() == "Available") << "Parallel: a completion changes nothing for siblings";
    EXPECT_FALSE(stage.isComplete()) << "Parallel: the stage waits for every child";
    finishItem(*b);
    finishItem(*c);
    EXPECT_TRUE(stage.isComplete()) << "Parallel: complete once all children are";
}

TEST(StageMode, SequentialMode) {
    CompositeTask stage("s", "Stage");
    useSequential(stage);
    LeafTask* a = addLeaf(stage, "a");
    LeafTask* b = addLeaf(stage, "b");
    LeafTask* c = addLeaf(stage, "c");

    stage.activate();
    EXPECT_TRUE(a->status() == "Available") << "Sequential: activate() releases the first child";
    EXPECT_TRUE(b->status() == "Created" && c->status() == "Created") << "Sequential: later children wait";

    finishItem(*a);
    EXPECT_TRUE(b->status() == "Available" && c->status() == "Created") << "Sequential: finishing child 1 releases child 2 only";
    EXPECT_TRUE(nearly(stage.progress(), 1.0 / 3.0)) << "Sequential: progress after one of three is 1/3";

    finishItem(*b);
    EXPECT_TRUE(c->status() == "Available") << "Sequential: finishing child 2 releases child 3";
    finishItem(*c);
    EXPECT_TRUE(stage.isComplete() && nearly(stage.progress(), 1.0)) << "Sequential: complete after the last child";
}

TEST(StageMode, SequentialWaitsForApproval) {
    CompositeTask stage("s", "Stage");
    useSequential(stage);
    LeafTask* a = addLeaf(stage, "a");
    LeafTask* b = addLeaf(stage, "b");
    stage.activate();

    a->assign("alice"); a->start(); a->complete();
    EXPECT_TRUE(a->status() == "AwaitingApproval" && b->status() == "Created") << "Sequential: work done but unapproved does not release the next child";

    a->reject("redo");
    EXPECT_TRUE(b->status() == "Created") << "Sequential: a rejection keeps the next child locked";

    a->start(); a->complete(); a->approve();
    EXPECT_TRUE(b->status() == "Available") << "Sequential: approval finally releases the next child";
}

TEST(StageMode, NestedStages) {
    // Sequential outer stage: sub-stage first, then a plain item.
    CompositeTask outer("outer", "Outer");
    useSequential(outer);
    CompositeTask* sub = addSubStage(outer, "sub");
    LeafTask* x = addLeaf(*sub, "x");
    LeafTask* y = addLeaf(*sub, "y");
    LeafTask* last = addLeaf(outer, "last");

    outer.activate();
    EXPECT_TRUE(x->status() == "Available" && y->status() == "Available") << "Nested: the first child stage is activated through its own mode";
    EXPECT_TRUE(last->status() == "Created") << "Nested: the item after the sub-stage waits";

    finishItem(*x);
    EXPECT_TRUE(last->status() == "Created") << "Nested: a half-finished sub-stage does not unlock the next child";
    finishItem(*y);
    EXPECT_TRUE(last->status() == "Available") << "Nested: a finished sub-stage unlocks the outer stage's next child";
    finishItem(*last);
    EXPECT_TRUE(outer.isComplete()) << "Nested: the outer stage completes";

    // Parallel outer stage holding a sequential sub-stage.
    CompositeTask top("top", "Top");
    CompositeTask* seq = addSubStage(top, "seq");
    useSequential(*seq);
    LeafTask* first = addLeaf(*seq, "first");
    LeafTask* second = addLeaf(*seq, "second");
    LeafTask* free = addLeaf(top, "free");

    top.activate();
    EXPECT_TRUE(first->status() == "Available" && second->status() == "Created") << "Nested: a sequential sub-stage inside a parallel stage keeps its order";
    EXPECT_TRUE(free->status() == "Available") << "Nested: its parallel sibling is released immediately";
}

TEST(StageMode, EdgeCases) {
    CompositeTask emptyParallel("p", "Empty");
    emptyParallel.activate();
    EXPECT_TRUE(emptyParallel.isComplete()) << "Edge: activating an empty parallel stage is harmless";

    CompositeTask emptySequential("q", "Empty");
    useSequential(emptySequential);
    emptySequential.activate();
    EXPECT_TRUE(emptySequential.isComplete()) << "Edge: activating an empty sequential stage is harmless";

    CompositeTask stage("s", "Stage");
    useSequential(stage);
    LeafTask* a = addLeaf(stage, "a");
    LeafTask* b = addLeaf(stage, "b");
    stage.activate();
    LeafTask stranger("zz", "Stranger", "Developer");
    SequentialMode mode;
    mode.childCompleted(stage, stranger);
    EXPECT_TRUE(a->status() == "Available" && b->status() == "Created") << "Edge: completion of a node outside the stage is ignored";

    CompositeTask parallel("pp", "Parallel");
    LeafTask* gone = addLeaf(parallel, "gone");
    LeafTask* live = addLeaf(parallel, "live");
    gone->cancel();
    parallel.activate();
    EXPECT_TRUE(gone->status() == "Cancelled") << "Edge: activation does not revive a cancelled child";
    EXPECT_TRUE(live->status() == "Available") << "Edge: other children are still released";
}
