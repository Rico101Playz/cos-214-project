#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "test_utils.h"
#include "state_test_helpers.h"

// delivery
// |-- build:   implement, test
// `-- release: review, deploy(prod)
static void buildTree(CompositeStage& root) {
    CompositeStage* build = addSubStage(root, "build");
    addLeaf(*build, "implement", "Developer");
    addLeaf(*build, "test", "QA");

    CompositeStage* release = addSubStage(root, "release");
    addLeaf(*release, "review", "Reviewer");
    CompositeStage* deploy = addSubStage(*release, "deploy");
    addLeaf(*deploy, "prod", "ReleaseManager");
}

static WorkItem* itemAt(CompositeStage& root, const std::string& id) {
    return static_cast<WorkItem*>(root.find(id));
}

static void test_BuildingTheTree() {
    std::cout << "\n--- Testing add() and child access ---\n";
    CompositeStage stage("s", "Stage");
    LeafWorkItem* leaf = new LeafWorkItem("l1", "Leaf", "Developer");

    WorkNode& added = stage.add(std::unique_ptr<WorkNode>(leaf));
    check(&added == leaf, "add: returns the node that was added");
    check(stage.childCount() == 1 && stage.child(0) == leaf, "add: the child is reachable by index");
    check(leaf->parent() == &stage, "add: the child's parent is the stage");
    check(stage.parent() == nullptr, "add: a root has no parent");

    addLeaf(stage, "l2");
    check(stage.childCount() == 2 && stage.child(1)->id() == "l2", "add: children keep their definition order");
    check(throwsInvalidArgument([&]{ stage.add(std::unique_ptr<WorkNode>()); }), "add: a null child throws");

    check(leaf->childCount() == 0 && leaf->child(0) == nullptr, "Leaf: has no children");
}

static void test_RemovingChildren() {
    std::cout << "\n--- Testing remove() ---\n";
    CompositeStage root("delivery", "Delivery");
    buildTree(root);
    CompositeStage* build = static_cast<CompositeStage*>(root.find("build"));

    check(build->remove("test") == true, "remove: removing a direct child succeeds");
    check(build->childCount() == 1 && root.find("test") == nullptr, "remove: the child is gone from the tree");
    check(build->remove("test") == false, "remove: removing it again reports false");

    check(root.remove("prod") == false, "remove: only direct children can be removed");
    check(root.find("prod") != nullptr, "remove: a refused removal changes nothing");

    CompositeStage* release = static_cast<CompositeStage*>(root.find("release"));
    check(release->remove("deploy") == true, "remove: a sub-stage can be removed");
    check(root.find("prod") == nullptr, "remove: removing a stage removes its whole subtree");
}

static void test_CompletionAndProgress() {
    std::cout << "\n--- Testing isComplete() and progress() ---\n";
    CompositeStage empty("e", "Empty");
    check(empty.isComplete() && nearly(empty.progress(), 1.0), "Progress: an empty stage is complete");

    CompositeStage root("delivery", "Delivery");
    buildTree(root);
    check(!root.isComplete() && nearly(root.progress(), 0.0), "Progress: a fresh tree is 0%");

    finishItem(*itemAt(root, "implement"));
    check(nearly(root.find("build")->progress(), 0.5), "Progress: a stage averages its children (1 of 2 = 0.5)");
    check(nearly(root.progress(), 0.25), "Progress: the root averages its stages ((0.5 + 0) / 2)");

    finishItem(*itemAt(root, "test"));
    check(root.find("build")->isComplete(), "Progress: a stage completes when its children do");
    check(!root.isComplete(), "Progress: the root waits for every stage");

    finishItem(*itemAt(root, "review"));
    finishItem(*itemAt(root, "prod"));
    check(root.isComplete() && nearly(root.progress(), 1.0), "Progress: the whole tree reports done through the root alone");
}

static void test_UniformInterface() {
    std::cout << "\n--- Testing that stages and leaves are treated uniformly ---\n";
    CompositeStage root("delivery", "Delivery");
    buildTree(root);

    std::vector<WorkNode*> nodes;
    nodes.push_back(&root);
    nodes.push_back(root.find("build"));
    nodes.push_back(root.find("implement"));

    bool allIncomplete = true;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->isComplete() || !nearly(nodes[i]->progress(), 0.0)) allIncomplete = false;
    }
    check(allIncomplete, "Uniform: root, stage and leaf all answer isComplete()/progress() the same way");

    finishItem(*itemAt(root, "implement"));
    check(nodes[2]->isComplete() && nearly(nodes[2]->progress(), 1.0), "Uniform: the leaf reports done through WorkNode*");
}

static void test_IsReady() {
    std::cout << "\n--- Testing isReady() ---\n";
    CompositeStage stage("s", "Stage");
    LeafWorkItem* leaf = addLeaf(stage, "l");
    check(!stage.isReady(), "Ready: a stage is never ready");
    check(!leaf->isReady(), "Ready: a Created leaf is not ready");
    leaf->makeAvailable();
    check(leaf->isReady(), "Ready: an Available leaf is ready");
    check(!stage.isReady(), "Ready: a stage stays not-ready even with ready children");
}

static void test_CancelCascade() {
    std::cout << "\n--- Testing cancel() cascading through the tree ---\n";
    CompositeStage root("delivery", "Delivery");
    buildTree(root);
    finishItem(*itemAt(root, "implement"));

    root.cancel();
    check(itemAt(root, "implement")->status() == "Completed", "Cancel: finished work is left alone");
    check(itemAt(root, "test")->status() == "Cancelled", "Cancel: reaches a leaf in the first stage");
    check(itemAt(root, "prod")->status() == "Cancelled", "Cancel: reaches a leaf several levels down");
    check(!root.isComplete(), "Cancel: a cancelled tree is not complete");
    check(nearly(root.progress(), 0.25), "Cancel: cancelled work counts for nothing");
}

static void test_Find() {
    std::cout << "\n--- Testing find() ---\n";
    CompositeStage root("delivery", "Delivery");
    buildTree(root);

    check(root.find("delivery") == &root, "Find: a node finds itself");
    WorkNode* prod = root.find("prod");
    check(prod != nullptr && prod->id() == "prod", "Find: reaches a deeply nested node");
    check(dynamic_cast<LeafWorkItem*>(prod) != nullptr, "Find: the node found is the real LeafWorkItem");
    check(root.find("nope") == nullptr, "Find: unknown id gives nullptr");
    check(root.find("deploy")->childCount() == 1, "Find: a found stage exposes its children");
    check(root.find("release")->find("implement") == nullptr, "Find: only searches below the node it is called on");
}

static void test_LeafMatchesPlainWorkItem() {
    std::cout << "\n--- Testing that LeafWorkItem is a drop-in for WorkItem ---\n";
    // Rejected -> start (rework) is a State-pattern extension, so it is not compared here.
    WorkItem plain("p", "Plain", "Developer");
    LeafWorkItem leaf("l", "Leaf", "Developer");

    check(plain.status() == leaf.status(), "Drop-in: same starting status");
    check(throwsLogicError([&]{ plain.start(); }) && throwsLogicError([&]{ leaf.start(); }),
          "Drop-in: both refuse start() while Created");

    plain.makeAvailable();  leaf.makeAvailable();
    check(plain.status() == leaf.status(), "Drop-in: same status after makeAvailable");
    plain.assign("amy");    leaf.assign("amy");
    check(plain.status() == leaf.status(), "Drop-in: same status after assign");
    plain.assign("ben");    leaf.assign("ben");
    check(plain.status() == leaf.status() && plain.assignee == leaf.assignee, "Drop-in: same behaviour on reassignment");
    plain.start();          leaf.start();
    plain.complete();       leaf.complete();
    check(plain.status() == leaf.status(), "Drop-in: same status after complete");
    plain.reject("redo");   leaf.reject("redo");
    check(plain.status() == leaf.status() && plain.reason() == leaf.reason(), "Drop-in: same status and reason after reject");
    plain.assign("cy");     leaf.assign("cy");
    plain.start();          leaf.start();
    plain.complete();       leaf.complete();
    plain.approve();        leaf.approve();
    check(plain.isComplete() == leaf.isComplete() && leaf.isComplete(), "Drop-in: both end Completed");
}

static void test_MixedChildren() {
    std::cout << "\n--- Testing a stage with plain WorkItems and LeafWorkItems ---\n";
    CompositeStage stage("s", "Mixed");
    WorkItem* plain = new WorkItem("plain", "Plain", "Developer");
    stage.add(std::unique_ptr<WorkNode>(plain));
    LeafWorkItem* leaf = addLeaf(stage, "leaf");

    finishItem(*plain);
    check(nearly(stage.progress(), 0.5), "Mixed: progress averages both kinds of child");
    check(!stage.isComplete(), "Mixed: not complete while the leaf is open");
    finishItem(*leaf);
    check(stage.isComplete(), "Mixed: complete when both kinds are done");
}

void run_composite_tests() {
    std::cout << "\n--- Running Composite Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_BuildingTheTree();
    test_RemovingChildren();
    test_CompletionAndProgress();
    test_UniformInterface();
    test_IsReady();
    test_CancelCascade();
    test_Find();
    test_LeafMatchesPlainWorkItem();
    test_MixedChildren();

    std::cout << "Composite Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}