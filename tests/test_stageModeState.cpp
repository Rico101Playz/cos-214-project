#include <iostream>
#include <memory>
#include <string>

#include "test_utils.h"
#include "state_test_helpers.h"

#include "patterns/execution/SequentialMode.h"
#include "patterns/execution/ParallelMode.h"

static void useSequential(CompositeStage& stage) {
    stage.setMode(std::unique_ptr<StageMode>(new SequentialMode()));
}

static void test_ModeNames() {
    std::cout << "\n--- Testing stage mode names and defaults ---\n";
    check(SequentialMode().name() == "Sequential", "SequentialMode: name()");
    check(ParallelMode().name() == "Parallel", "ParallelMode: name()");

    CompositeStage stage("s", "Stage");
    check(stage.mode().name() == "Parallel", "CompositeStage: defaults to Parallel");
    useSequential(stage);
    check(stage.mode().name() == "Sequential", "CompositeStage: setMode() switches the mode");
    stage.setMode(std::unique_ptr<StageMode>());
    check(stage.mode().name() == "Parallel", "CompositeStage: clearing the mode falls back to Parallel");
}

static void test_ParallelMode() {
    std::cout << "\n--- Testing ParallelMode ---\n";
    CompositeStage stage("s", "Stage");
    LeafWorkItem* a = addLeaf(stage, "a");
    LeafWorkItem* b = addLeaf(stage, "b");
    LeafWorkItem* c = addLeaf(stage, "c");

    stage.activate();
    check(a->status() == "Available" && b->status() == "Available" && c->status() == "Available",
          "Parallel: activate() releases every child at once");

    finishItem(*a);
    check(b->status() == "Available" && c->status() == "Available", "Parallel: a completion changes nothing for siblings");
    check(!stage.isComplete(), "Parallel: the stage waits for every child");
    finishItem(*b);
    finishItem(*c);
    check(stage.isComplete(), "Parallel: complete once all children are");
}

static void test_SequentialMode() {
    std::cout << "\n--- Testing SequentialMode ---\n";
    CompositeStage stage("s", "Stage");
    useSequential(stage);
    LeafWorkItem* a = addLeaf(stage, "a");
    LeafWorkItem* b = addLeaf(stage, "b");
    LeafWorkItem* c = addLeaf(stage, "c");

    stage.activate();
    check(a->status() == "Available", "Sequential: activate() releases the first child");
    check(b->status() == "Created" && c->status() == "Created", "Sequential: later children wait");

    finishItem(*a);
    check(b->status() == "Available" && c->status() == "Created", "Sequential: finishing child 1 releases child 2 only");
    check(nearly(stage.progress(), 1.0 / 3.0), "Sequential: progress after one of three is 1/3");

    finishItem(*b);
    check(c->status() == "Available", "Sequential: finishing child 2 releases child 3");
    finishItem(*c);
    check(stage.isComplete() && nearly(stage.progress(), 1.0), "Sequential: complete after the last child");
}

static void test_SequentialWaitsForApproval() {
    std::cout << "\n--- Testing that sequential waits for approval, not just work done ---\n";
    CompositeStage stage("s", "Stage");
    useSequential(stage);
    LeafWorkItem* a = addLeaf(stage, "a");
    LeafWorkItem* b = addLeaf(stage, "b");
    stage.activate();

    a->assign("alice"); a->start(); a->complete();
    check(a->status() == "AwaitingApproval" && b->status() == "Created",
          "Sequential: work done but unapproved does not release the next child");

    a->reject("redo");
    check(b->status() == "Created", "Sequential: a rejection keeps the next child locked");

    a->start(); a->complete(); a->approve();
    check(b->status() == "Available", "Sequential: approval finally releases the next child");
}

static void test_NestedStages() {
    std::cout << "\n--- Testing nested stages ---\n";
    // Sequential outer stage: sub-stage first, then a plain item.
    CompositeStage outer("outer", "Outer");
    useSequential(outer);
    CompositeStage* sub = addSubStage(outer, "sub");
    LeafWorkItem* x = addLeaf(*sub, "x");
    LeafWorkItem* y = addLeaf(*sub, "y");
    LeafWorkItem* last = addLeaf(outer, "last");

    outer.activate();
    check(x->status() == "Available" && y->status() == "Available", "Nested: the first child stage is activated through its own mode");
    check(last->status() == "Created", "Nested: the item after the sub-stage waits");

    finishItem(*x);
    check(last->status() == "Created", "Nested: a half-finished sub-stage does not unlock the next child");
    finishItem(*y);
    check(last->status() == "Available", "Nested: a finished sub-stage unlocks the outer stage's next child");
    finishItem(*last);
    check(outer.isComplete(), "Nested: the outer stage completes");

    // Parallel outer stage holding a sequential sub-stage.
    CompositeStage top("top", "Top");
    CompositeStage* seq = addSubStage(top, "seq");
    useSequential(*seq);
    LeafWorkItem* first = addLeaf(*seq, "first");
    LeafWorkItem* second = addLeaf(*seq, "second");
    LeafWorkItem* free = addLeaf(top, "free");

    top.activate();
    check(first->status() == "Available" && second->status() == "Created", "Nested: a sequential sub-stage inside a parallel stage keeps its order");
    check(free->status() == "Available", "Nested: its parallel sibling is released immediately");
}

static void test_EdgeCases() {
    std::cout << "\n--- Testing edge cases ---\n";
    CompositeStage emptyParallel("p", "Empty");
    emptyParallel.activate();
    check(emptyParallel.isComplete(), "Edge: activating an empty parallel stage is harmless");

    CompositeStage emptySequential("q", "Empty");
    useSequential(emptySequential);
    emptySequential.activate();
    check(emptySequential.isComplete(), "Edge: activating an empty sequential stage is harmless");

    CompositeStage stage("s", "Stage");
    useSequential(stage);
    LeafWorkItem* a = addLeaf(stage, "a");
    LeafWorkItem* b = addLeaf(stage, "b");
    stage.activate();
    LeafWorkItem stranger("zz", "Stranger", "Developer");
    SequentialMode mode;
    mode.childCompleted(stage, stranger);
    check(a->status() == "Available" && b->status() == "Created", "Edge: completion of a node outside the stage is ignored");

    CompositeStage parallel("pp", "Parallel");
    LeafWorkItem* gone = addLeaf(parallel, "gone");
    LeafWorkItem* live = addLeaf(parallel, "live");
    gone->cancel();
    parallel.activate();
    check(gone->status() == "Cancelled", "Edge: activation does not revive a cancelled child");
    check(live->status() == "Available", "Edge: other children are still released");
}

void run_stage_mode_tests() {
    std::cout << "\n--- Running Stage Mode Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_ModeNames();
    test_ParallelMode();
    test_SequentialMode();
    test_SequentialWaitsForApproval();
    test_NestedStages();
    test_EdgeCases();

    std::cout << "Stage Mode Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}