#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "state_test_helpers.h"

// delivery
// |-- build:   implement, test
// `-- release: review, deploy(prod)
static void buildTree(CompositeTask& root) {
    CompositeTask* build = addSubStage(root, "build");
    addLeaf(*build, "implement", "Developer");
    addLeaf(*build, "test", "QA");

    CompositeTask* release = addSubStage(root, "release");
    addLeaf(*release, "review", "Reviewer");
    CompositeTask* deploy = addSubStage(*release, "deploy");
    addLeaf(*deploy, "prod", "ReleaseManager");
}

static LeafTask* itemAt(CompositeTask& root, const std::string& id) {
    return static_cast<LeafTask*>(root.find(id));
}

TEST(CompositeTree, BuildingTheTree) {
    CompositeTask stage("s", "Stage");
    LeafTask* leaf = new LeafTask("l1", "Leaf", "Developer");

    Task& added = stage.add(std::unique_ptr<Task>(leaf));
    EXPECT_EQ(&added, leaf) << "add: returns the node that was added";
    EXPECT_TRUE(stage.childCount() == 1 && stage.child(0) == leaf) << "add: the child is reachable by index";
    EXPECT_TRUE(leaf->parent() == &stage) << "add: the child's parent is the stage";
    EXPECT_EQ(stage.parent(), nullptr) << "add: a root has no parent";

    addLeaf(stage, "l2");
    EXPECT_TRUE(stage.childCount() == 2 && stage.child(1)->id() == "l2") << "add: children keep their definition order";
    EXPECT_THROW(([&]{ stage.add(std::unique_ptr<Task>()); })(), std::invalid_argument) << "add: a null child throws";

    EXPECT_TRUE(leaf->childCount() == 0 && leaf->child(0) == nullptr) << "Leaf: has no children";
}

TEST(CompositeTree, RemovingChildren) {
    CompositeTask root("delivery", "Delivery");
    buildTree(root);
    CompositeTask* build = static_cast<CompositeTask*>(root.find("build"));

    EXPECT_TRUE(build->remove("test") == true) << "remove: removing a direct child succeeds";
    EXPECT_TRUE(build->childCount() == 1 && root.find("test") == nullptr) << "remove: the child is gone from the tree";
    EXPECT_TRUE(build->remove("test") == false) << "remove: removing it again reports false";

    EXPECT_FALSE(root.remove("prod")) << "remove: only direct children can be removed";
    EXPECT_NE(root.find("prod"), nullptr) << "remove: a refused removal changes nothing";

    CompositeTask* release = static_cast<CompositeTask*>(root.find("release"));
    EXPECT_TRUE(release->remove("deploy") == true) << "remove: a sub-stage can be removed";
    EXPECT_EQ(root.find("prod"), nullptr) << "remove: removing a stage removes its whole subtree";
}

TEST(CompositeTree, CompletionAndProgress) {
    CompositeTask empty("e", "Empty");
    EXPECT_TRUE(empty.isComplete() && nearly(empty.progress(), 1.0)) << "Progress: an empty stage is complete";

    CompositeTask root("delivery", "Delivery");
    buildTree(root);
    EXPECT_TRUE(!root.isComplete() && nearly(root.progress(), 0.0)) << "Progress: a fresh tree is 0%";

    finishItem(*itemAt(root, "implement"));
    EXPECT_TRUE(nearly(root.find("build")->progress(), 0.5)) << "Progress: a stage averages its children (1 of 2 = 0.5)";
    EXPECT_TRUE(nearly(root.progress(), 0.25)) << "Progress: the root averages its stages ((0.5 + 0) / 2)";

    finishItem(*itemAt(root, "test"));
    EXPECT_TRUE(root.find("build")->isComplete()) << "Progress: a stage completes when its children do";
    EXPECT_FALSE(root.isComplete()) << "Progress: the root waits for every stage";

    finishItem(*itemAt(root, "review"));
    finishItem(*itemAt(root, "prod"));
    EXPECT_TRUE(root.isComplete() && nearly(root.progress(), 1.0)) << "Progress: the whole tree reports done through the root alone";
}

TEST(CompositeTree, UniformInterface) {
    CompositeTask root("delivery", "Delivery");
    buildTree(root);

    std::vector<Task*> nodes;
    nodes.push_back(&root);
    nodes.push_back(root.find("build"));
    nodes.push_back(root.find("implement"));

    bool allIncomplete = true;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->isComplete() || !nearly(nodes[i]->progress(), 0.0)) allIncomplete = false;
    }
    EXPECT_TRUE(allIncomplete) << "Uniform: root, stage and leaf all answer isComplete()/progress() the same way";

    finishItem(*itemAt(root, "implement"));
    EXPECT_TRUE(nodes[2]->isComplete() && nearly(nodes[2]->progress(), 1.0)) << "Uniform: the leaf reports done through Task*";
}

TEST(CompositeTree, IsReady) {
    CompositeTask stage("s", "Stage");
    LeafTask* leaf = addLeaf(stage, "l");
    EXPECT_FALSE(stage.isReady()) << "Ready: a stage is never ready";
    EXPECT_TRUE(!leaf->isReady()) << "Ready: a Created leaf is not ready";
    leaf->makeAvailable();
    EXPECT_TRUE(leaf->isReady()) << "Ready: an Available leaf is ready";
    EXPECT_FALSE(stage.isReady()) << "Ready: a stage stays not-ready even with ready children";
}

TEST(CompositeTree, CancelCascade) {
    CompositeTask root("delivery", "Delivery");
    buildTree(root);
    finishItem(*itemAt(root, "implement"));

    root.cancel();
    EXPECT_TRUE(itemAt(root, "implement")->status() == "Completed") << "Cancel: finished work is left alone";
    EXPECT_TRUE(itemAt(root, "test")->status() == "Cancelled") << "Cancel: reaches a leaf in the first stage";
    EXPECT_TRUE(itemAt(root, "prod")->status() == "Cancelled") << "Cancel: reaches a leaf several levels down";
    EXPECT_FALSE(root.isComplete()) << "Cancel: a cancelled tree is not complete";
    EXPECT_TRUE(nearly(root.progress(), 0.25)) << "Cancel: cancelled work counts for nothing";
}

TEST(CompositeTree, Find) {
    CompositeTask root("delivery", "Delivery");
    buildTree(root);

    EXPECT_EQ(root.find("delivery"), &root) << "Find: a node finds itself";
    Task* prod = root.find("prod");
    EXPECT_TRUE(prod != nullptr && prod->id() == "prod") << "Find: reaches a deeply nested node";
    EXPECT_TRUE(leafOf(prod) != nullptr) << "Find: the node found is the real LeafTask";
    EXPECT_EQ(root.find("nope"), nullptr) << "Find: unknown id gives nullptr";
    EXPECT_TRUE(root.find("deploy")->childCount() == 1) << "Find: a found stage exposes its children";
    EXPECT_TRUE(root.find("release")->find("implement") == nullptr) << "Find: only searches below the node it is called on";
}

TEST(CompositeTree, LeafMatchesPlainWorkItem) {
    // Rejected -> start (rework) is a State-pattern extension, so it is not compared here.
    LeafTask plain("p", "Plain", "Developer");
    LeafTask leaf("l", "Leaf", "Developer");

    EXPECT_EQ(plain.status(), leaf.status()) << "Drop-in: same starting status";
    EXPECT_TRUE(throwsLogicError([&]{ plain.start(); }) && throwsLogicError([&]{ leaf.start(); })) << "Drop-in: both refuse start() while Created";

    plain.makeAvailable();  leaf.makeAvailable();
    EXPECT_EQ(plain.status(), leaf.status()) << "Drop-in: same status after makeAvailable";
    plain.assign("amy");    leaf.assign("amy");
    EXPECT_EQ(plain.status(), leaf.status()) << "Drop-in: same status after assign";
    plain.assign("ben");    leaf.assign("ben");
    EXPECT_TRUE(plain.status() == leaf.status() && plain.assignee() == leaf.assignee()) << "Drop-in: same behaviour on reassignment";
    plain.start();          leaf.start();
    plain.complete();       leaf.complete();
    EXPECT_EQ(plain.status(), leaf.status()) << "Drop-in: same status after complete";
    plain.reject("redo");   leaf.reject("redo");
    EXPECT_TRUE(plain.status() == leaf.status() && plain.reason() == leaf.reason()) << "Drop-in: same status and reason after reject";
    plain.assign("cy");     leaf.assign("cy");
    plain.start();          leaf.start();
    plain.complete();       leaf.complete();
    plain.approve();        leaf.approve();
    EXPECT_TRUE(plain.isComplete() == leaf.isComplete() && leaf.isComplete()) << "Drop-in: both end Completed";
}

TEST(CompositeTree, MixedChildren) {
    CompositeTask stage("s", "Mixed");
    LeafTask* plain = new LeafTask("plain", "Plain", "Developer");
    stage.add(std::unique_ptr<Task>(plain));
    LeafTask* leaf = addLeaf(stage, "leaf");

    finishItem(*plain);
    EXPECT_TRUE(nearly(stage.progress(), 0.5)) << "Mixed: progress averages both kinds of child";
    EXPECT_FALSE(stage.isComplete()) << "Mixed: not complete while the leaf is open";
    finishItem(*leaf);
    EXPECT_TRUE(stage.isComplete()) << "Mixed: complete when both kinds are done";
}
